param(
	[Parameter(Mandatory = $true)]
	[string]$SmlProjectDir,

	[Parameter(Mandatory = $true)]
	[string]$UnrealEngineDir,

	[switch]$SkipLinuxServer
)

$ErrorActionPreference = "Stop"

$ModDir = Split-Path -Parent $PSScriptRoot
$TargetModDir = Join-Path $SmlProjectDir "Mods\MapItemFinder"
$UProject = Join-Path $SmlProjectDir "FactoryGame.uproject"
$RunUAT = Join-Path $UnrealEngineDir "Engine\Build\BatchFiles\RunUAT.bat"
$OutputDir = Join-Path $ModDir "dist"

Write-Host "Syncing MapItemFinder into $TargetModDir..."
if (Test-Path $TargetModDir) {
	Remove-Item -Recurse -Force $TargetModDir
}
New-Item -ItemType Directory -Path $TargetModDir | Out-Null
Copy-Item (Join-Path $ModDir "MapItemFinder.uplugin") $TargetModDir
Copy-Item -Recurse (Join-Path $ModDir "Source") (Join-Path $TargetModDir "Source")

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Write-Host "Building Win64 client target..."
& $RunUAT PackagePlugin `
	-Project="$UProject" `
	-Plugin="$TargetModDir\MapItemFinder.uplugin" `
	-TargetPlatforms=Win64 `
	-TargetConfigurations=Shipping `
	-Package="$OutputDir\Win64"
if ($LASTEXITCODE -ne 0) { throw "Win64 build failed" }

if (-not $SkipLinuxServer) {
	Write-Host "Building LinuxServer dedicated server target..."
	& $RunUAT PackagePlugin `
		-Project="$UProject" `
		-Plugin="$TargetModDir\MapItemFinder.uplugin" `
		-TargetPlatforms=LinuxServer `
		-TargetConfigurations=Shipping `
		-Package="$OutputDir\LinuxServer"
	if ($LASTEXITCODE -ne 0) { throw "LinuxServer build failed" }
}

Write-Host "Done. Packaged output is in $OutputDir"
