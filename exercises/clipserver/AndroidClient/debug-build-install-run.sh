ant debug
adb shell pm uninstall com.example.clipbook
adb install bin/Clipbook-debug.apk
adb shell am start -n com.example.clipbook/com.example.clipbook.MainActivity
