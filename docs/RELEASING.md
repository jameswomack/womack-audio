# Releasing the Womack Audio installer

`scripts/package.sh` builds every plugin, code-signs the bundles, packages them
into a single installer, and notarizes + staples it:

```
dist/WomackAudio-<version>-installer.pkg   # Womack FX + Womack Resonote, AU + VST3
```

There are two ways to produce a release: **locally** on a Mac, or via the
**`Release` GitHub Actions workflow**.

---

## Prerequisites (both paths)

- A paid **Apple Developer Program** membership. Your **Team ID** is at
  <https://developer.apple.com/account> → Membership (10 chars, e.g. `AB12CD34EF`).
- Two **Developer ID** certificates:
  - **Developer ID Application** — signs the `.component` / `.vst3` bundles.
  - **Developer ID Installer** — signs the `.pkg`.
  Create both from **Xcode → Settings → Accounts → <team> → Manage Certificates → `+`**
  (this stores them, with private keys, in your login keychain).
- An **app-specific password** for notarization: <https://appleid.apple.com> →
  Sign-In and Security → App-Specific Passwords → `+`.

---

## Local release

`package.sh` reads five environment variables:

| Variable | What it is | How to find it |
|---|---|---|
| `APP_CERT_NAME` | Developer ID **Application** identity | `security find-identity -v -p codesigning` |
| `INSTALLER_CERT_NAME` | Developer ID **Installer** identity | `security find-identity -v \| grep "Developer ID Installer"` |
| `APPLE_ID` | Apple ID email | — |
| `APPLE_TEAM_ID` | 10-char Team ID | developer.apple.com → Membership |
| `APPLE_APP_PASSWORD` | app-specific password | appleid.apple.com |

```bash
export APP_CERT_NAME="Developer ID Application: Your Name (AB12CD34EF)"
export INSTALLER_CERT_NAME="Developer ID Installer: Your Name (AB12CD34EF)"
export APPLE_ID="you@example.com"
export APPLE_TEAM_ID="AB12CD34EF"
export APPLE_APP_PASSWORD="abcd-efgh-ijkl-mnop"

scripts/package.sh 1.0.0
```

The hardened runtime (`--options runtime`) and a secure timestamp are applied
automatically — both required for notarization. `--wait` blocks until Apple
returns a verdict (usually 1–5 minutes).

Verify the result:
```bash
spctl -a -vvv -t install dist/WomackAudio-1.0.0-installer.pkg
# expect: accepted / source=Notarized Developer ID
```
If notarization is rejected, read the log:
```bash
xcrun notarytool log <submission-id> \
  --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD"
```

---

## CI release (`.github/workflows/release.yml`)

Triggers on a `v*` tag push (attaches the installer to a GitHub Release) or via
**Actions → Release → Run workflow** (uploads the installer as a build artifact).

### One-time: add repository secrets

Settings → Secrets and variables → Actions → **New repository secret**:

| Secret | Value |
|---|---|
| `DEVELOPER_ID_APP_P12_BASE64` | base64 of the exported Developer ID **Application** `.p12` |
| `DEVELOPER_ID_INSTALLER_P12_BASE64` | base64 of the exported Developer ID **Installer** `.p12` |
| `P12_PASSWORD` | password you set when exporting the `.p12` files (use the same for both) |
| `KEYCHAIN_PASSWORD` | any random string (temp keychain password) |
| `APP_CERT_NAME` | `Developer ID Application: Your Name (TEAMID)` |
| `INSTALLER_CERT_NAME` | `Developer ID Installer: Your Name (TEAMID)` |
| `APPLE_ID` | Apple ID email |
| `APPLE_TEAM_ID` | 10-char Team ID |
| `APPLE_APP_PASSWORD` | app-specific password |

Export each certificate (with its private key) from **Keychain Access → right-click
→ Export → `.p12`**, then base64-encode for the secret:
```bash
base64 -i DeveloperID_Application.p12 | pbcopy   # paste into DEVELOPER_ID_APP_P12_BASE64
base64 -i DeveloperID_Installer.p12   | pbcopy   # paste into DEVELOPER_ID_INSTALLER_P12_BASE64
```

### Cut a release
```bash
git tag v1.0.0
git push origin v1.0.0
```
The workflow imports the certs into a temporary keychain, runs `scripts/package.sh`,
and attaches `dist/WomackAudio-1.0.0-installer.pkg` to the `v1.0.0` GitHub Release.
