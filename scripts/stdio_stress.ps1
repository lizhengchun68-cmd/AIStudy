# M6: stdio stability — invoke AIstudy N times with one JSON envelope per process.
# Usage: .\scripts\stdio_stress.ps1 [-Count 100] [-Exe path\to\AIstudy.exe]
param(
    [int]$Count = 100,
    [string]$Exe = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if (-not $Exe) {
    $Exe = Join-Path $root "build\src\Release\AIstudy.exe"
}
$requestPath = Join-Path $root "skills\rainflow\tests\request_ok.json"
if (-not (Test-Path $Exe)) {
    Write-Error "AIstudy not found: $Exe (build Release first)"
}
if (-not (Test-Path $requestPath)) {
    Write-Error "Missing $requestPath"
}

$envelope = Get-Content $requestPath -Raw
$fail = 0
$sw = [System.Diagnostics.Stopwatch]::StartNew()

function Invoke-AIstudyStdio {
    param([string]$JsonEnvelope)
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $Exe
    $psi.WorkingDirectory = $root
    $psi.UseShellExecute = $false
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $proc = [System.Diagnostics.Process]::Start($psi)
    $proc.StandardInput.Write($JsonEnvelope)
    $proc.StandardInput.Close()
    $stdout = $proc.StandardOutput.ReadToEnd()
    [void]$proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
    return @{ Out = $stdout.Trim(); Code = $proc.ExitCode }
}

for ($i = 1; $i -le $Count; $i++) {
    $r = Invoke-AIstudyStdio -JsonEnvelope $envelope
    if ($r.Code -ne 0) {
        Write-Host "[$i] exit=$($r.Code)"
        $fail++
        continue
    }
    try {
        $obj = $r.Out | ConvertFrom-Json
        if (-not $obj.ok) {
            Write-Host "[$i] ok=false body=$($r.Out)"
            $fail++
        }
    } catch {
        Write-Host "[$i] invalid json: $($r.Out)"
        $fail++
    }
}

$sw.Stop()
Write-Host "stdio_stress: total=$Count fail=$fail elapsed=$([math]::Round($sw.Elapsed.TotalSeconds,1))s"
if ($fail -gt 0) { exit 1 }
exit 0
