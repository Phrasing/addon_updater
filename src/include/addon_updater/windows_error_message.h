#ifndef ADDON_UPDATER_WINDOWS_ERROR_MESSAGE_H
#define ADDON_UPDATER_WINDOWS_ERROR_MESSAGE_H

namespace addon_updater {
std::string WindowsErrorMessage(DWORD error_code);
void WindowsErrorMessageBox(std::string_view message);
}

#endif  // !ADDON_UPDATER_WINDOWS_ERROR_MESSAGE_H
