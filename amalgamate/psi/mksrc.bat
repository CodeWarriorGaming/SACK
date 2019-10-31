:::
:   
:   
:   

@set SRCS=


@set SRCS= 
@set SRCS= %SRCS%   ../../src/psilib/controls.c
@set SRCS= %SRCS%   ../../src/psilib/borders.c
@set SRCS= %SRCS%   ../../src/psilib/progress_bar.c
@set SRCS= %SRCS%   ../../src/psilib/caption_buttons.c
@set SRCS= %SRCS%   ../../src/psilib/control_physical.c
@set SRCS= %SRCS%   ../../src/psilib/ctlbutton.c

@set SRCS= %SRCS%   ../../src/psilib/ctlcombo.c
@set SRCS= %SRCS%   ../../src/psilib/ctledit.c
@set SRCS= %SRCS%   ../../src/psilib/ctlimage.c
@set SRCS= %SRCS%   ../../src/psilib/ctllistbox.c
@set SRCS= %SRCS%   ../../src/psilib/ctlmisc.c
@set SRCS= %SRCS%   ../../src/psilib/ctlprop.c
@set SRCS= %SRCS%   ../../src/psilib/ctlscroll.c
@set SRCS= %SRCS%   ../../src/psilib/ctlsheet.c
@set SRCS= %SRCS%   ../../src/psilib/ctlslider.c
@set SRCS= %SRCS%   ../../src/psilib/ctltext.c
@set SRCS= %SRCS%   ../../src/psilib/ctltooltip.c
@set SRCS= %SRCS%   ../../src/psilib/fileopen.c
@set SRCS= %SRCS%   ../../src/psilib/fntdlg.c
:@set SRCS= %SRCS%   ../../src/psilib/loadsave.c
@set SRCS= %SRCS%   ../../src/psilib/mouse.c
@set SRCS= %SRCS%   ../../src/psilib/palette.c
@set SRCS= %SRCS%   ../../src/psilib/popups.c
@set SRCS= %SRCS%   ../../src/psilib/xml_load.c
@set SRCS= %SRCS%   ../../src/psilib/xml_save.c
@set SRCS= %SRCS%   ../../src/psilib/option_frame.c
@set SRCS= %SRCS%   ../../src/psilib/scrollknob.c

@set SRCS= %SRCS%   ../../src/psilib/fntcache.c

@set SRCS= %SRCS%   ../../src/psilib/console/console_block_writer.c
@set SRCS= %SRCS%   ../../src/psilib/console/console_keydefs.c
@set SRCS= %SRCS%   ../../src/psilib/console/history.c
@set SRCS= %SRCS%   ../../src/psilib/console/paste.c
@set SRCS= %SRCS%   ../../src/psilib/console/psicon.c
@set SRCS= %SRCS%   ../../src/psilib/console/regaccess.c
@set SRCS= %SRCS%   ../../src/psilib/console/WinLogic.c
@set SRCS= %SRCS%   ../../src/psilib/console/psicon_interface.c

@set SRCS= %SRCS%   ../../src/psilib/calctl/calender.c
@set SRCS= %SRCS%   ../../src/psilib/calctl/clock.c
@set SRCS= %SRCS%   ../../src/psilib/calctl/analog.c
 
@set SRCS= %SRCS%   ../../src/systraylib/systray.c

@set SRCS= %SRCS%   ../../src/SQLlib/optlib/editoption/editopt.c

del sack_ucb_psi.h
del sack_ucb_psi.c

set OPTS=
set OPTS=%OPTS% -I../../src/contrib/freetype-2.8/include

c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include %OPTS% -p -osack_ucb_psi.c -DINCLUDE_LOGGING %SRCS%

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% ../../../include/controls.h
@set HDRS= %HDRS% ../../../include/psi/namespaces.h
@set HDRS= %HDRS% ../../../include/psi.h
@set HDRS= %HDRS% ../../../include/psi/buttons.h
@set HDRS= %HDRS% ../../../include/psi/clock.h
@set HDRS= %HDRS% ../../../include/psi/console.h
@set HDRS= %HDRS% ../../../include/psi/edit.h
@set HDRS= %HDRS% ../../../include/psi/knob.h
@set HDRS= %HDRS% ../../../include/psi/shadewell.h
@set HDRS= %HDRS% ../../../include/psi/slider.h
@set HDRS= %HDRS% ../../../include/systray.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -o../sack_ucb_psi.h %HDRS%
cd ..



