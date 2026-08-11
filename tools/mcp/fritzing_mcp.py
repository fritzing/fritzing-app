# /// script
# requires-python = ">=3.11"
# dependencies = [
#     "mcp>=1.2,<2",
#     "httpx",
# ]
# ///
"""MCP server for driving a running Fritzing instance.

Fritzing must be running with its testing server enabled:

    Fritzing -ftesting

This bridge translates MCP tool calls into HTTP requests against the
FTesting probe server (http://127.0.0.1:17999), specifically the McpSketch
probe's "call" verb. Register with Claude Code:

    claude mcp add fritzing -- uv run /path/to/tools/mcp/fritzing_mcp.py
"""

import json
import os
from urllib.parse import quote

import httpx
from mcp.server.fastmcp import FastMCP, Image

BASE_URL = os.environ.get("FRITZING_TESTING_URL", "http://127.0.0.1:17999")
TIMEOUT = 120.0

mcp = FastMCP(
    "fritzing",
    instructions="""Draw and inspect breadboard circuit diagrams in a live Fritzing window.

Typical workflow:
1. search_parts to find module_ids (e.g. "breadboard", "LED", "resistor", "Arduino Uno").
   Well-known core parts: full+ breadboard "Breadboard-RSR03MB102-ModuleID",
   red 5mm LED "5mmColorLEDModuleID", 220 ohm resistor "ResistorModuleID".
2. add_part to place parts. Coordinates are breadboard-view scene units (~90 per inch);
   the part is centered on (x, y). Place parts on clear free spots, roughly 100-400 units
   apart, and keep the sketch compact.
3. get_connectors on each placed part to find connector ids and names for wiring
   (e.g. an LED's anode/cathode, an Arduino's "D13/SCK" and "GND"). Prefer it over
   get_part_info for wiring: some connectors listed in the part definition exist
   only in the schematic/pcb views and cannot be wired on the breadboard.
4. connect_parts to wire connectors together. Wires are straight lines; electrical
   correctness matters more than hole-perfect placement.
5. get_sketch_state to verify parts and connections, export_image to see the result.

Prefer delete_item + add_part over move_part when repositioning a part that is
plugged into breadboard holes: move does not re-form hole connections.""",
)


def _call(tool: str, args: dict) -> dict:
    request = json.dumps({"tool": tool, "args": args})
    url = f"{BASE_URL}/McpSketch/call/{quote(request, safe='')}"
    try:
        response = httpx.get(url, timeout=TIMEOUT)
    except httpx.ConnectError as exc:
        raise RuntimeError(
            "Cannot reach Fritzing. Start it with the testing server enabled: "
            "Fritzing -ftesting"
        ) from exc
    if response.status_code != 200:
        raise RuntimeError(f"Fritzing returned HTTP {response.status_code}: {response.text}")
    result = json.loads(response.text)
    if not result.get("ok", False):
        detail = f"{result.get('error', 'error')}: {result.get('detail', '')}"
        if "available" in result:
            detail += f" (available connectors: {', '.join(result['available'])})"
        raise RuntimeError(detail)
    result.pop("ok", None)
    return result


@mcp.tool()
def fritzing_status() -> str:
    """Check that Fritzing is running with its automation server enabled."""
    try:
        response = httpx.get(f"{BASE_URL}/McpSketch/read", timeout=10.0)
    except httpx.ConnectError:
        return "Fritzing is NOT reachable. Start it with: Fritzing -ftesting"
    return f"Fritzing is running. {response.text}"


@mcp.tool()
def search_parts(query: str, max_results: int = 10) -> str:
    """Search the Fritzing parts library by free text (e.g. "LED", "resistor",
    "Arduino Uno", "breadboard"). Returns matching parts with their module_id,
    which other tools use to identify the part type."""
    return json.dumps(_call("search_parts", {"query": query, "max_results": max_results}))


@mcp.tool()
def get_part_info(module_id: str) -> str:
    """Get a part's title, description, properties, and connector list (id, name,
    description per connector) from its definition. Good for choosing parts; for
    wiring, prefer get_connectors on the placed item, since some defined connectors
    are not present in the breadboard view."""
    return json.dumps(_call("get_part_info", {"module_id": module_id}))


@mcp.tool()
def add_part(module_id: str, x: float, y: float, rotation: float = 0) -> str:
    """Add a part to the breadboard view, centered on scene coordinates (x, y)
    (~90 units per inch; +x right, +y down). Returns the new item_id used by all
    other tools. If a part's pins land exactly on breadboard holes they connect
    automatically, but the simpler robust approach is to place parts on free
    space and wire them with connect_parts."""
    return json.dumps(_call("add_part", {"module_id": module_id, "x": x, "y": y, "rotation": rotation}))


@mcp.tool()
def connect_parts(
    from_item_id: int,
    from_connector: str,
    to_item_id: int,
    to_connector: str,
    color: str = "",
) -> str:
    """Create a wire in the breadboard view between two connectors, e.g. from an
    Arduino pin to an LED leg. Connector ids come from get_part_info or
    get_connectors. Optional wire color: red, black, blue, green, yellow, white.
    Returns the wire's item id. To disconnect, delete_item the wire."""
    args = {
        "from_item_id": from_item_id,
        "from_connector": from_connector,
        "to_item_id": to_item_id,
        "to_connector": to_connector,
    }
    if color:
        args["color"] = color
    return json.dumps(_call("connect", args))


@mcp.tool()
def move_part(item_id: int, x: float, y: float) -> str:
    """Move a part so its center lands on scene coordinates (x, y). Note: moving
    does NOT re-form breadboard hole connections; prefer delete_item + add_part
    for parts seated in breadboard holes. Explicit wires stay attached."""
    return json.dumps(_call("move", {"item_id": item_id, "x": x, "y": y}))


@mcp.tool()
def rotate_part(item_id: int, degrees: float) -> str:
    """Rotate a part by the given number of degrees (positive = clockwise)."""
    return json.dumps(_call("rotate", {"item_id": item_id, "degrees": degrees}))


@mcp.tool()
def delete_item(item_id: int) -> str:
    """Delete a part or wire from the sketch (deleting a wire disconnects it)."""
    return json.dumps(_call("delete", {"item_id": item_id}))


@mcp.tool()
def get_sketch_state() -> str:
    """Get the current sketch as JSON: all parts (item_id, module_id, title,
    refdes, center x/y, connections) and all wires with their endpoints. Use this
    to verify the circuit's connectivity after building it."""
    return json.dumps(_call("get_sketch_state", {}))


@mcp.tool()
def get_connectors(item_id: int, id_prefix: str = "") -> str:
    """List a placed part's connectors (id, name, scene position) in the breadboard
    view. This is the authoritative list for wiring. For big parts like a breadboard
    (hundreds of holes) use id_prefix to filter; hole naming varies by part, e.g.
    "pin53H" on the full+ breadboard, "A10" on the half breadboard."""
    args = {"item_id": item_id}
    if id_prefix:
        args["id_prefix"] = id_prefix
    return json.dumps(_call("get_connectors", args))


@mcp.tool()
def export_image(view: str = "breadboard", dpi: int = 100) -> Image:
    """Render the sketch to a PNG image and return it, so you can visually
    inspect what the circuit looks like. view: breadboard, schematic, or pcb.
    Keep dpi modest (100 or less) unless you need to zoom into details."""
    result = _call("export_image", {"view": view, "format": "png", "dpi": dpi})
    with open(result["path"], "rb") as f:
        return Image(data=f.read(), format="png")


@mcp.tool()
def export_image_to_file(path: str, view: str = "breadboard", format: str = "png", dpi: int = 300) -> str:
    """Export the sketch to a PNG or SVG file at the given absolute path."""
    return json.dumps(_call("export_image", {"view": view, "format": format, "dpi": dpi, "path": path}))


@mcp.tool()
def save_sketch(path: str) -> str:
    """Save the sketch as a shareable .fzz file at the given absolute path."""
    return json.dumps(_call("save_sketch", {"path": path}))


@mcp.tool()
def open_sketch(path: str) -> str:
    """Open a .fzz or .fz sketch file in Fritzing."""
    return json.dumps(_call("open_sketch", {"path": path}))


if __name__ == "__main__":
    mcp.run()
