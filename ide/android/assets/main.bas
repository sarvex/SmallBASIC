const app = "main.bas?"
const boldOn = chr(27) + "[1m"
const boldOff = chr(27) + "[21m"
const char_h = txth("Q")
const lineSpacing = 2 + char_h
const onlineUrl = "http://smallbasic.sourceforge.net/?q=export/code/1243"
const idxEdit = 5
const idxFiles = 6

func spaced(s)
  local ch, len_s
  len_s = len(s)
  local out = ""
  for ch = 1 to len_s
    out += mid(s, ch, 1) + " "
  next ch
  spaced = out
end

func mk_bn(value, lab, fg)
  local bn
  bn.x = 0
  bn.y = -lineSpacing
  bn.value = value
  bn.label = lab
  bn.color = fg
  mk_bn = bn
end

func mk_menu(value, lab, x)
  local bn
  bn.x = x
  bn.y = 0
  bn.value = value
  bn.label = lab
  bn.color = 7
  bn.backgroundColor = 1
  bn.type = "tab"
  mk_menu = bn
end

sub intro(byref frm)
  local i, bn
  for i = 1 to 4
    bn = mk_bn(0, "Welcome to SmallBASIC", i)
    bn.type = "label"
    frm.inputs << bn
  next i
  bn = mk_bn(0, spaced("Welcome to SmallBASIC"), 7)
  bn.type = "label"
  frm.inputs << bn
end

sub do_okay_button()
  local frm, button
  button.x = xmax / 2
  button.y = -1
  button.label = "Close"
  button.backgroundColor = "blue"
  button.color = "white"
  frm.inputs << button
  frm = form(frm)
  print
  frm.doEvents()
end

sub do_about()
  cls
  print " __           _      ___ _"
  print "(_ ._ _  _.|||_) /\ (_ |/ "
  print "__)| | |(_||||_)/--\__)|\_"
  print
  print "Version 0.12.5"
  print
  print "Copyright (c) 2002-2015 Chris Warren-Smith"
  print "Copyright (c) 1999-2006 Nic Christopoulos" + chr(10)
  print "http://smallbasic.sourceforge.net" + chr(10)
  print "SmallBASIC comes with ABSOLUTELY NO WARRANTY. ";
  print "This program is free software; you can use it ";
  print "redistribute it and/or modify it under the terms of the ";
  print "GNU General Public License version 2 as published by ";
  print "the Free Software Foundation." + chr(10)
  print "Envy Code R Font v0.8 used with permission ";
  print "http://damieng.com/envy-code-r" + chr(10)
  print
  server_info()
  do_okay_button()
  cls
end

sub do_setup()
  color 3, 0
  cls
  print boldOn + "Setup web service port number."
  print boldOff
  print "Enter a port number to allow web browser or desktop IDE access. ";
  print "Enter -1 to disable this feature, or press <enter> to leave ";
  print "this screen without making any changes."
  print "The current setting is: " + env("serverSocket")
  print
  color 15, 3
  input socket
  if (len(socket) > 0) then
    env("serverSocket=" + socket)
    randomize timer
    token = ""
    for i = 1 to 6
      token += chr (asc("A") + ((rnd * 1000) % 20))
    next i
    env("serverToken=" + token)
    local msg = "You must restart SmallBASIC for this change to take effect"
    local wnd = window()
    wnd.alert(msg, "Restart required")
  endif
  color 7, 0
  cls
end

sub server_info()
  local serverSocket = env("serverSocket")
  local ipAddr = env("IP_ADDR")

  if (len(serverSocket) > 0 && len(ipAddr)) then
    serverSocket = ipAddr + ":" + serverSocket
    print boldOff + "Web Service: " + boldOn + serverSocket
    print boldOff + "Access token: " + boldOn + env("serverToken")
    print boldOff
  fi
end

func fileCmpFunc(l, r)
  local f1 = lower(l)
  local f2 = lower(r)
  fileCmpFunc = IFF(f1 == f2, 0, IFF(f1 > f2, 1, -1))
end

sub listFiles(byref frm, path, byref basList, byref dirList)
  local fileList, ent, name, lastItem, bn, bn_back

  erase basList
  erase dirList

  if (right(path, 1) != "/") then
    path += "/"
  endif

  bn = mk_bn(0, "Files in " + path, 7)
  bn.type = "label"
  bn.x = 0
  bn.y = -lineSpacing
  frm.inputs << bn

  bn_back = mk_bn("_back", "[Go up]", 3)
  bn_back.type = "link"
  bn_back.x = 0
  bn_back.y = -lineSpacing
  frm.inputs << bn_back

  fileList = files(path)

  for ent in fileList
    name = ent
    if (isdir(path + name) && left(name, 1) != ".") then
      dirList << name
    else if (lower(right(ent, 4)) == ".bas") then
      basList << name
    endif
  next ent

  sort dirList use fileCmpFunc(x,y)
  sort basList use filecmpfunc(x,y)

  lastItem = len(dirList) - 1

  for i = 0 to lastItem
    bn = mk_bn(path + dirList(i), "[" + dirList(i) + "]", 3)
    bn.type = "link"
    frm.inputs << bn
  next ent

  lastItem = len(basList) - 1
  for i = 0 to lastItem
    bn = mk_bn(path + basList(i), basList(i), 2)
    bn.type = "link"
    bn.isExit = true
    frm.inputs << bn
  next ent
end

func getFiles()
  local list = files("*.*")
  local entry

  dim result
  for entry in list
    if (lower(right(entry, 4)) == ".bas") then
      result << entry
    endIf
  next entry

  sort result use fileCmpFunc(x,y)
  getFiles = result
end

sub createNewFile(byref f, byref wnd)
  f.refresh(true)
  local newFile = f.inputs(idxEdit).value
  if (len(newFile) == 0) then
    exit sub
  endIf
  if (lower(right(newFile, 4)) != ".bas") then
    newFile += ".bas"
  endIf
  try
    if (exist(newFile)) then
      wnd.alert("File " + newFile + " already exists", "Duplicate File")
    else
      dim text
      text << "REM SmallBASIC"
      text << "REM created: " + date
      tsave newFile, text
    endif
  catch e
    wnd.alert("Error creating file: " + e)
  end try
end

sub manageFiles()
  local f, wnd, bn_edit, bn_files
  const renameId = "__bn_rename__"
  const deleteId = "__bn_delete__"
  const newId = "__bn_new__"
  const viewId = "__bn_view__"
  const closeId = "__bn_close__"

  sub mk_item(x, lab, value)
    local bn
    bn.x = x
    bn.y = 0
    bn.label = lab
    bn.value = value
    bn.backgroundColor = 1
    bn.color = 7
    bn.type = "tab"
    f.inputs << bn
  end

  sub createUI()
    cls
    rect 0, 0, xmax, lineSpacing COLOR 1 filled
    mk_item( 0, "Home", closeId)
    mk_item(-1, "View", viewId)
    mk_item(-1, "Rename", renameId)
    mk_item(-1, "New", newId)
    mk_item(-1, "Delete", deleteId)
    bn_edit.x = 0
    bn_edit.y = char_h + 4
    bn_edit.width = xmax
    bn_edit.type = "text"
    bn_edit.color = "white"
    bn_files.x = x1
    bn_files.y = bn_edit.y + char_h + 2
    bn_files.height = ymax - bn_files.y
    bn_files.width = xmax - x1
    bn_files.color = 2
    bn_files.type = "list"
    f.inputs << bn_edit
    f.inputs << bn_files
    f.focusColor = "white"
    f = form(f)
    f.value = bn_edit.value
  end

  sub reloadList(selectedIndex)
    local f_list = getFiles()
    local f_list_len=len(f_list)
    if (f_list_len == 0) then
      selectedFile = ""
      f.inputs(idxFiles).value = ""
      selectedIndex = 0
    else
      if (selectedIndex == f_list_len) then
        selectedIndex--
      endif
      selectedFile = f_list(selectedIndex)
      f.inputs(idxFiles).value = f_list
    endif
    f.inputs(idxFiles).selectedIndex = selectedIndex
    f.inputs(idxEdit).value = selectedFile
    f.refresh(false)
  end

  sub deleteFile()
    wnd.ask("Are you sure you wish to delete " + selectedFile + "?", "Delete File")
    if (wnd.answer == 0) then
      f.refresh(true)
      local selectedIndex = f.inputs(idxFiles).selectedIndex
      kill selectedFile
      reloadList(selectedIndex)
    endif
    f.value = ""
  end

  sub renameFile()
    ' retrieve the edit value
    f.refresh(true)
    local newFile = f.inputs(idxEdit).value
    local selectedIndex = f.inputs(idxFiles).selectedIndex
    if (lower(right(newFile, 4)) != ".bas") then
      newFile += ".bas"
    endIf

    if (exist(selectedFile) and selectedFile != newFile) then
      rename selectedFile, newFile
      reloadList(selectedIndex)
    endif
    f.value = selectedFile
  end

  sub viewFile()
    local frm, button

    if (!exist(selectedFile)) then
      wnd.alert("Select a file and try again")
    else
      tload selectedFile, buffer
      wnd.graphicsScreen2()
      cls
      len_buffer = len(buffer) - 1
      for i = 0 to len_buffer
        print buffer(i)
      next i
      do_okay_button
      wnd.graphicsScreen1()
      f.value = selectedFile
    endIf
  end

  createUI()
  reloadList(0)
  wnd = window()

  while 1
    f.doEvents()
    select case f.value
    case renameId
      renameFile()
    case deleteId
      deleteFile()
    case newId
      createNewFile(f, wnd)
      reloadList(0)
    case viewId
      viewFile()
    case closeId
      exit loop
    case else
      if (len(f.value) > 0) then
        ' set the edit value
        f.inputs(idxEdit).value = f.value
        f.refresh(false)
        selectedFile = f.value
      endif
    end select
  wend
  cls
end

sub main
  local basList, dirList, path
  local frm, bn_about, bn_online, bn_new
  local do_intro

  dim basList
  dim dirList

  bn_files = mk_menu("_files", "File", 0)
  bn_online = mk_menu(onlineUrl, "Online", -1)
  bn_setup = mk_menu("_setup", "Setup", -1)
  bn_about = mk_menu("_about", "About", -1)
  bn_online.isExit = true

  func makeUI(path, welcome)
    local frm
    frm.inputs << bn_files
    frm.inputs << bn_online
    if (osname != "SDL") then
      frm.inputs << bn_setup
    endif
    frm.inputs << bn_about

    if (welcome) then
      intro(frm)
    fi

    listFiles frm, path, basList, dirList
    frm.color = 10
    rect 0, 0, xmax, lineSpacing COLOR 1 filled
    at 0, 0
    frm = form(frm)
    makeUI = frm
  end

  sub go_back
    local backPath, index
    backPath = ""
    index = iff(isstring(path), rinstr(path, "/"), 0)
    if (index > 0 && index == len(path)) then
      index = rinstr(left(path, index - 1), "/")
    fi
    if (index == 1) then
      index++
    fi
    if (index > 0)
      backPath = left(path, index - 1)
    else
      backPath = "/"
    endif
    path = backPath
  end

  do_intro = false
  if (command == "welcome") then
    do_intro = true
  fi
  path = cwd
  frm = makeUI(path, do_intro)

  while 1
    frm.doEvents()

    if (isdir(frm.value)) then
      frm.close()
      path = frm.value
      chdir path
      frm = makeUI(path, false)
    elif frm.value == "_about" then
      frm.close()
      do_about()
      frm = makeUI(path, false)
    elif frm.value == "_setup" then
      frm.close()
      do_setup()
      frm = makeUI(path, false)
    elif frm.value == "_files" then
      frm.close()
      managefiles()
      frm = makeUI(path, false)
    elif frm.value == "_back" then
      frm.close()
      go_back()
      frm = makeUI(path, false)
    fi
  wend
end

main
