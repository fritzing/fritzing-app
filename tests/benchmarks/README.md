# Breadboard autoroute benchmarks

The sweep runner opens every input sketch from a clean Fritzing process and ranks
results using the required objective:

1. zero failed nets;
2. minimum jumper count;
3. minimum total jumper length;
4. minimum component-lead length;
5. congestion and elapsed time as diagnostics only.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\benchmarks\breadboard_autoroute_sweep.ps1 `
  -FzzFiles F:\docs\Fritzing\fuzz.fzz,F:\docs\Fritzing\another-circuit.fzz
```

The runner writes `artifacts/breadboard-autoroute-sweep.csv`. Each run starts from
the saved `.fzz`; generated wires and placement from an earlier run cannot leak
into the next result.
