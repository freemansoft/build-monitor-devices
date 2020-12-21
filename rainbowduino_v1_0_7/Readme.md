See the Readme.md at the top of this repo for more informatin about this project.


Blog articles on the hardware for these devices may be available at http://joe.blog.freemansoft.com/
Source located at https://github.com/freemansoft/build-monitor-devices

```
                              unsigned char cmd[8] 8 byte array
                              cmd[0] cmd[1] cmd[2] cmd[3] cmd[4] cmd[5] cmd[6]
display specific picture.     R'     0      shift  NA     NA     NA     index
display specific character.   R'     1      shift  red    green. blue   ASCII
display speicific color.      R'     2      NA     red    green  blue   NA
set specific led dot          R'     3      line   column red    green  blue
run specific flash demo.      R'     4      index  NA     NA     NA     NA
3d dot                        R'     5      xyz    red    green  blue   0
change to data mode           R'     D'     NA     NA     NA     NA     NA
```
