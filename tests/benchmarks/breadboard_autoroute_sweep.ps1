param(
    [Parameter(Mandatory = $true)]
    [string[]]$FzzFiles,
    [string]$ExePath = "F:\src\fritzing-app\release64\Fritzing.exe",
    [string[]]$MaxJumperLengths = @("180", "240", "320"),
    [string[]]$CandidatesPerBusPair = @("4", "8", "12"),
    [string[]]$CrossingPenalties = @("0", "700"),
    [int]$LoadTimeoutSeconds = 45,
    [string]$OutputCsv = "F:\src\fritzing-app\artifacts\breadboard-autoroute-sweep.csv",
    [string]$ScreenshotDirectory = "F:\src\fritzing-app\artifacts\breadboard-autoroute-screenshots"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

function Stop-Fritzing {
    Get-Process Fritzing -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
}

function Wait-MainWindow([System.Diagnostics.Process]$Process, [int]$TimeoutSeconds) {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    do {
        $Process.Refresh()
        if ($Process.HasExited) { throw "Fritzing exited before its window opened." }
        if ($Process.MainWindowHandle -ne 0) { return }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)
    throw "Timed out waiting for Fritzing to load."
}

function Wait-DocumentLoaded([System.Diagnostics.Process]$Process, [string]$DocumentPath, [int]$TimeoutSeconds) {
    $expectedName = [IO.Path]::GetFileName($DocumentPath)
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    do {
        $Process.Refresh()
        if ($Process.HasExited) { throw "Fritzing exited before the document loaded." }
        if ($Process.MainWindowTitle -like "*$expectedName*") {
            Start-Sleep -Seconds 2
            return
        }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)
    throw "Timed out waiting for Fritzing to load $expectedName."
}

function Find-AutorouteButton([System.Diagnostics.Process]$Process, [int]$TimeoutSeconds) {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty,
        "Autoroute")
    do {
        $Process.Refresh()
        if ($Process.HasExited) { throw "Fritzing exited before Autoroute became available." }
        $root = [System.Windows.Automation.AutomationElement]::FromHandle($Process.MainWindowHandle)
        $button = $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
        if ($button) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if (-not $button) { throw "Timed out waiting for the Autoroute button." }
    return $button
}

function Invoke-Autoroute([System.Diagnostics.Process]$Process, [int]$TimeoutSeconds) {
    $button = Find-AutorouteButton $Process $TimeoutSeconds
    $invoke = $button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
    $invoke.Invoke()
}

function Save-WindowScreenshot([System.Diagnostics.Process]$Process, [string]$Path) {
    $Process.Refresh()
    if ($Process.HasExited -or $Process.MainWindowHandle -eq 0) {
        throw "Cannot capture a Fritzing window that is not open."
    }

    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class BreadboardBenchmarkWindow {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
}
"@ -ErrorAction SilentlyContinue

    [BreadboardBenchmarkWindow]::SetForegroundWindow($Process.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 250
    $rect = New-Object BreadboardBenchmarkWindow+RECT
    if (-not [BreadboardBenchmarkWindow]::GetWindowRect($Process.MainWindowHandle, [ref]$rect)) {
        throw "GetWindowRect failed for Fritzing."
    }
    $width = [Math]::Max(1, $rect.Right - $rect.Left)
    $height = [Math]::Max(1, $rect.Bottom - $rect.Top)
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $hdc = $graphics.GetHdc()
        try {
            [BreadboardBenchmarkWindow]::PrintWindow($Process.MainWindowHandle, $hdc, 2) | Out-Null
        }
        finally {
            $graphics.ReleaseHdc($hdc)
        }
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Wait-Benchmark([string]$LogPath, [int]$TimeoutSeconds) {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    do {
        if (Test-Path -LiteralPath $LogPath) {
            $line = Get-Content -LiteralPath $LogPath |
                Where-Object { $_ -match " benchmark: " } |
                Select-Object -Last 1
            if ($line) { return }
        }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)
    throw "Timed out waiting for the autoroute benchmark result."
}

function Invoke-Undo([System.Diagnostics.Process]$Process) {
    [BreadboardBenchmarkWindow]::SetForegroundWindow($Process.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 200
    [System.Windows.Forms.SendKeys]::SendWait("^z")
    Start-Sleep -Seconds 2
}

function Parse-Benchmark([string]$LogPath) {
    $line = Get-Content -LiteralPath $LogPath |
        Where-Object { $_ -match " benchmark: " } |
        Select-Object -Last 1
    if (-not $line) { throw "Autoroute did not emit a benchmark result." }

    $pattern = 'failedNets=(\d+) jumperCount=(\d+) jumperLength=([0-9.]+) componentLeadLength=([0-9.]+) congestion=([0-9.]+) elapsedMs=(\d+)'
    if ($line -notmatch $pattern) { throw "Unrecognized benchmark line: $line" }
    return @{
        FailedNets = [int]$Matches[1]
        JumperCount = [int]$Matches[2]
        JumperLength = [double]$Matches[3]
        ComponentLeadLength = [double]$Matches[4]
        Congestion = [double]$Matches[5]
        ElapsedMs = [int]$Matches[6]
    }
}

if (-not (Test-Path -LiteralPath $ExePath)) { throw "Fritzing executable not found: $ExePath" }
$results = [System.Collections.Generic.List[object]]::new()
$logPath = Join-Path $env:TEMP "fritzing-breadboard-autorouter.log"
$expandedFzzFiles = @($FzzFiles | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$expandedMaxLengths = @($MaxJumperLengths | ForEach-Object { $_ -split ',' } | ForEach-Object { [double]$_ })
$expandedCandidateCounts = @($CandidatesPerBusPair | ForEach-Object { $_ -split ',' } | ForEach-Object { [int]$_ })
$expandedCrossingPenalties = @($CrossingPenalties | ForEach-Object { $_ -split ',' } | ForEach-Object { [double]$_ })
New-Item -ItemType Directory -Force -Path $ScreenshotDirectory | Out-Null
Get-ChildItem -LiteralPath $ScreenshotDirectory -Filter "*.png" -ErrorAction SilentlyContinue | Remove-Item -Force

try {
    foreach ($fzz in $expandedFzzFiles) {
        if (-not (Test-Path -LiteralPath $fzz)) { throw "FZZ file not found: $fzz" }
        foreach ($maxLength in $expandedMaxLengths) {
            foreach ($candidateCount in $expandedCandidateCounts) {
                foreach ($crossingPenalty in $expandedCrossingPenalties) {
                    Stop-Fritzing
                    Remove-Item -LiteralPath $logPath -Force -ErrorAction SilentlyContinue
                    $env:FRITZING_BB_MAX_JUMPER_LENGTH = $maxLength
                    $env:FRITZING_BB_CANDIDATES_PER_BUS_PAIR = $candidateCount
                    $env:FRITZING_BB_CROSSING_PENALTY = $crossingPenalty

                    $process = Start-Process -FilePath $ExePath -ArgumentList @("`"$fzz`"") -PassThru
                    Wait-MainWindow $process $LoadTimeoutSeconds
                    Wait-DocumentLoaded $process $fzz $LoadTimeoutSeconds
                    Find-AutorouteButton $process $LoadTimeoutSeconds | Out-Null
                    $caseName = "{0}-L{1}-C{2}-X{3}" -f ([IO.Path]::GetFileNameWithoutExtension($fzz)), $maxLength, $candidateCount, $crossingPenalty
                    Save-WindowScreenshot $process (Join-Path $ScreenshotDirectory "$caseName-before.png")
                    Invoke-Autoroute $process $LoadTimeoutSeconds
                    Wait-Benchmark $logPath $LoadTimeoutSeconds
                    $metrics = Parse-Benchmark $logPath
                    Save-WindowScreenshot $process (Join-Path $ScreenshotDirectory "$caseName-autorouted.png")
                    Invoke-Undo $process
                    Save-WindowScreenshot $process (Join-Path $ScreenshotDirectory "$caseName-undo.png")

                    $results.Add([pscustomobject]@{
                        Fzz = (Resolve-Path -LiteralPath $fzz).Path
                        MaxJumperLength = $maxLength
                        CandidatesPerBusPair = $candidateCount
                        CrossingPenalty = $crossingPenalty
                        FailedNets = $metrics.FailedNets
                        JumperCount = $metrics.JumperCount
                        JumperLength = $metrics.JumperLength
                        ComponentLeadLength = $metrics.ComponentLeadLength
                        Congestion = $metrics.Congestion
                        ElapsedMs = $metrics.ElapsedMs
                    })
                }
            }
        }
    }
}
finally {
    Stop-Fritzing
    Remove-Item Env:FRITZING_BB_MAX_JUMPER_LENGTH -ErrorAction SilentlyContinue
    Remove-Item Env:FRITZING_BB_CANDIDATES_PER_BUS_PAIR -ErrorAction SilentlyContinue
    Remove-Item Env:FRITZING_BB_CROSSING_PENALTY -ErrorAction SilentlyContinue
}

$ranked = $results | Sort-Object Fzz, FailedNets, JumperCount, JumperLength, ComponentLeadLength, Congestion, ElapsedMs
$outputDirectory = Split-Path -Parent $OutputCsv
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$ranked | Export-Csv -LiteralPath $OutputCsv -NoTypeInformation
$ranked | Format-Table -AutoSize
Write-Host "Results: $OutputCsv"
