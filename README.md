# Capture Browser (Qt6 WebEngine)

A lightweight, portable browser built with Qt 6.8.0 and WebEngine. This project features persistent cookie storage, Google Auth compatibility, and an automated Windows build system.

## Features
* **Normal Save**: Persistent cookie and session management via a dedicated storage path.
* **Google Login Fix**: Modified User-Agent to bypass "Not Secure" browser blocks.
* **Portable Mode**: All user data is stored in the `/user_data` folder relative to the executable.
* **Developer Tools**: Remote debugging enabled on port 9222.

## How to Build for Windows (Automated)
This project uses **GitHub Actions**. You do not need a Windows PC to build the `.exe`.
1. Push this code to a GitHub repository.
2. Ensure the `.github/workflows/windows_build.yml` file is present.
3. Go to the **Actions** tab on GitHub.
4. Download the `Windows-Browser-Portable` zip file from the latest successful build.

## Running the App
### Windows
1. Download and extract the build zip.
2. Run `Capture.exe`.
3. Your login data will be saved in the `user_data` folder created in the same directory.

### Linux (Debian/Ubuntu)
If cookies are not saving and you see an "off-the-record" folder:
1. Run `rm -rf $HOME/.pki/nssdb && mkdir -p $HOME/.pki/nssdb`
2. Run `certutil -d sql:$HOME/.pki/nssdb -N --empty-password`
3. Restart the browser.

## Tech Stack
* **Framework**: Qt 6.8.0
* **Engine**: Chromium (QtWebEngine)
* **Build System**: QMake / MSVC (via GitHub Actions)
