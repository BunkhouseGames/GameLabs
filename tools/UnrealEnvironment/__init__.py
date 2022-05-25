import winreg

def get_unreal_path_from_config(version):
    try:
        registry_path = f"SOFTWARE\\EpicGames\\Unreal Engine\\{version}"
        name = "InstalledDirectory"

        registry_key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, registry_path, 0, winreg.KEY_READ)
        value, regtype = winreg.QueryValueEx(registry_key, name)
        winreg.CloseKey(registry_key)

        print(f"Unreal Engine {version} installed at {value}")
        return value

    except WindowsError:
        print(f"Error: Unreal Engine {version} not found in registry")
        return None



