$env:MSYSTEM = ""
$env:MSYS = ""
$env:IDF_PATH = "C:\env\Espressif\frameworks\esp-idf-v5.5.1"
$env:IDF_PYTHON_ENV_PATH = "C:\env\Espressif\python_env\idf5.5_py3.11_env"

# Source ESP-IDF environment (sets up all toolchain paths)
. "$env:IDF_PATH\export.ps1"

Set-Location "D:\Program\MekCraft-Labs\embedded-lab\esp32\CameraCtrl"
& idf.py build