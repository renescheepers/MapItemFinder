# Map Item Finder

A lightweight Satisfactory (SML `^3.12.1`, SMM `v3.2.0`) mod that adds a search bar to the in-game map so you can find where any item is **produced**, **extracted**, or **stored** (containers & Dimensional Depots) across multiplayer and dedicated servers.

## How it works

- **Dedicated server / Host (authoritative):** When you pick an item in the map search bar, the client sends a reliable Server RPC (`UMapItemFinderRCO::Server_SearchItem`). The server scans `AFGBuildableSubsystem` (manufacturers, extractors, storage containers, and Dimensional Depots), clusters nearby machines within 25 m into a single pin, and sends the compact result list back to the requesting player.
- **Per-player map pins:** Results render as local-only `UFGActorRepresentation` map markers (green = produced, cyan = extracted, amber = stored, purple = Dimensional Depot) so your search never clutters other players' maps.
- **Client-only fallback:** `AcceptsAnyRemoteVersion` is enabled; if the server does not have the mod installed, the client falls back to scanning locally replicated buildables.
- **Pure C++ + Slate:** No `.uasset` Blueprints required. All Slate UI code is guarded with `#if !UE_SERVER` so `LinuxServer` builds cleanly.

## Building on Windows (Win64 + LinuxServer)

Prerequisites on your Windows build PC (Satisfactory does not need to be installed to compile):

1. [Satisfactory Modding Unreal Engine (CSS-UE5)](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/dependencies.html) + Visual Studio 2022 (C++ desktop workload).
2. UE5 Linux Cross-Compile Toolchain (for `LinuxServer` dedicated server builds).
3. Cloned [SatisfactoryModLoader](https://github.com/satisfactorymodding/SatisfactoryModLoader) starter project.

Build from PowerShell:

```powershell
.\scripts\build-windows.ps1 `
  -SmlProjectDir "C:\Dev\SatisfactoryModLoader" `
  -UnrealEngineDir "C:\Dev\UnrealEngine-CSS"
```

Or copy this folder into `<SatisfactoryModLoader>\Mods\MapItemFinder`, open `FactoryGame.uproject`, and package with **Alpakit** (selecting both `Windows` and `LinuxServer`) to produce the `.smod` archive for Satisfactory Mod Manager (SMM).
