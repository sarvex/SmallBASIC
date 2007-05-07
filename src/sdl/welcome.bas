REM $Id: welcome.bas,v 1.1 2007-05-07 01:23:18 zeeb90au Exp $

sub slow_print(s)
   local ch, len_s
   len_s = len(s)
   for ch = 1 to len_s 
      ? mid(s, ch, 1) + " ";
      delay 80
   next ch
end

sub intro()
  e = chr(27)+"["
  ? e+"90m"+e+"31m"+"Welcome to SmallBASIC"
  ? e+"91m"+e+"32m"+"Welcome to SmallBASIC"
  ? e+"92m"+e+"33m"+"Welcome to SmallBASIC"
  ? e+"93m"+e+"34m"+"Welcome to SmallBASIC"
  ? e+"0m"+e+"90m"
  slow_print "Welcome to SmallBASIC"
  ? chr$(27)+"[90m"
end

cls
intro
delay 1500

open_bn = ""
load_bn = ""
quit_bn = ""
edit_lb = "? 'Hello World'"

margin = xmax/3

doform 0, 0, xmax, ymax
list = files("*.bas")
button  5, 5, margin, -1, list, "", "listbox"
button -5, 5, -1, -1, open_bn, "Run"
button -5, 5, -1, -1, load_bn, "Load"
button -5, 5, -1, -1, quit_bn, "Quit"
text margin+10, -4, xmax-(margin+14), ymax-28, edit_lb
list = 0

while 1
   doform
   if (open_bn = "Run") then
      open_bn = ""
      cls
      if exist(list)
        tload list, code
        chain code
      fi
   elif (load_bn = "Load") then
       load_bn = ""
       if exist(list)
         tload list, code
         edit_lb = code
       fi
   elif (quit_bn = "Quit") then
      exit loop
   fi
wend


