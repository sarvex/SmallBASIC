s="1234567"
? replace$(s, 2, "bcdefg")
? replace$(s, 2, "b", 2)
? replace$(s, 2, "b", 5)
? replace$(s, 2, "", len(s))
? replace$(s, 2, "bc", 0)
? replace$(s, 2, "bcI", 2)
?
? leftof$(s, "23")
? rightof$(s, "23")
? leftoflast$(s, "23")
? rightoflast$(s, "23")

rem ------------------------------------
rem REPLACE str argument can now be a number type
a="1234"
num=9
b=replace(a, 2, num, 2)
if (b!="194") then
  throw "replace err"
endif

