# CI and Releases

The repository has two GitHub Actions workflows. Neither runs on every push.

## Build (`.github/workflows/build.yml`)

Runs on **pull requests** and when started manually (**Actions → Build → Run workflow**).

| Job | What it does |
|---|---|
| Version | Resolves the version: the workflow input, or the `VERSION` file |
| Windows (x86) | `build.ps1 -Platform Windows -Package`, then checks the DLL exports with `dumpbin` |
| Linux (x86) | `build.ps1 -Platform Linux -Package` on Ubuntu (Zig cross-compiler), checked by `tools/elfcheck.py` |
| GitHub release | Only when requested (see below) |

The open.mp SDK and Zig are cached between runs. Every run uploads two artifacts, `weapon-forge-X.Y.Z-windows` and `weapon-forge-X.Y.Z-linux`. Their download contains `plugins/`, `components/` and `include/` directly, with no archive inside. The `.zip` / `.tar.gz` archives are only created for releases.

### Publishing a release

1. **Actions → Build → Run workflow**.
2. Tick **"Create a GitHub release with the binaries"**.
3. Optionally fill the version (e.g. `1.2.0`; empty uses the `VERSION` file) and tick pre-release.
4. Run.

The release job creates the tag `vX.Y.Z` on the commit that was built and attaches:

- `weapon-forge-X.Y.Z-windows.zip`
- `weapon-forge-X.Y.Z-linux.tar.gz`
- `weapon-forge.inc`

The release notes contain the installation table plus GitHub's automatically generated changelog. The job fails if the tag already exists.

Remember to update the `VERSION` file when bumping the version, so local builds and CI agree.

## Wiki (`.github/workflows/wiki.yml`)

Publishes the pages in the repository's `wiki/` folder to its GitHub wiki. It only runs when started manually (**Actions → Wiki → Run workflow**).

One-time setup: enable the wiki in **Settings → General → Features → Wikis** and create any first page in the web UI. GitHub only creates the wiki's git repository after that first page exists. Every run then replaces the wiki content with the files in `wiki/`.
