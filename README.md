#


## cpu
linux: `top`
macos: `ps -A | awk`

## memory 
linux: `free`
macos: `vm_stat`

## network
linux: `/proc/net/dev`

## Very Useful Resources:

* [reddit post by u/BLACKSourceCode](https://www.reddit.com/r/MechanicalKeyboards/comments/bysjcy/oled_used_for_displaying_dynamic_info/)
* [u/BLACKSourceCode hid server (node-js)](https://github.com/BlankSourceCode/qmk-hid-display)
* [linux hidraw example (Very Useful)](https://github.com/torvalds/linux/blob/master/samples/hidraw/hid-example.c)
* [useful nvdia-smi queries](https://nvidia.custhelp.com/app/answers/detail/a_id/3751/~/useful-nvidia-smi-queries)
* [helixfonteditor (Very Useful)](https://helixfonteditor.netlify.com/)
* [qmk oled API](https://github.com/qmk/qmk_firmware/blob/master/docs/feature_oled_driver.md)
* [hidraw permission](https://github.com/node-hid/node-hid#devicewritedata)
* [USB HID documentation](https://www.usb.org/hid)
* [C++ popen example](https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po)
