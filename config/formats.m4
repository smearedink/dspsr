dnl @synopsis DSPSR_TEST_FORMAT([format])
dnl
AC_DEFUN([DSPSR_TEST_FORMAT],
[
  AC_PROVIDE([DSPSR_TEST_FORMAT])

  [$1]_selected=0;
  for sel_format in x $selected_formats; do
    if test [$1] = $sel_format; then
      [$1]_selected=1;
    fi
  done
])

dnl @synopsis DSPSR_SELECTED_FORMATS
dnl 
AC_DEFUN([DSPSR_SELECTED_FORMATS],
[
  AC_PROVIDE([DSPSR_SELECTED_FORMATS])

  # formats.list will always be in the current working directory when configure is run
  selected_formats=`cat formats.list 2> /dev/null`

  if test x"$selected_formats" = x; then
    AC_MSG_WARN([

    No file formats selected.  Please see http://dspsr.sourceforge.net/formats

    ])
  else
    AC_MSG_NOTICE([Selected formats: $selected_formats])
    AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_builddir)/formats.list'])
  fi

  AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/config/formats.sh'])

])

