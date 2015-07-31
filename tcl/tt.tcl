#
# ThoughtTreasure commonsense platform
# Tcl-based client API
#
# Copyright 1998 Erik Thomas Mueller. All Rights Reserved.
#
# 19981202T171511: begun
# 19981203T140703: more work
# 19981204T084800: eliminate newlines
#
# Examples:
# set tth [TT_Open somehost 8888]
# [TT_ISA $tth "beverage" "Evian"]
# [TT_Retrieve $tth 2 1 -1 "anc" "duration-of attend-play ?"]
# [TT_Tag $tth "z" "eat breakfast"]
# [TT_HowManyMinutesIs $tth "tennis w/Susanna"]
#

# higher-level functions

proc TT_HowManyMinutesIs {tth s} {
  if {$s == ""} {
    return 30
  }
  set pns [TT_Tag $tth "z" $s]
  set sum 0
  set count 0
  foreach {pn} $pns {
    set objname [lindex $pn 4]
    set objs [TT_Retrieve $tth 2 1 -1 "anc" "duration-of $objname ?"]
    foreach {obj} $objs {
      set minutes [TT_GetMinutes $obj]
      if {$minutes < 720} {
        set sum [expr $sum + $minutes]
        set count [expr $count + 1]
      }
    }
  }
  if {$count == 0} {
    return 30
  }
  return [expr $sum / $count]
}

proc TT_GetMinutes {s} {
  set t [TT_Tokenize $s ":"]
  return [expr [lindex $t 2] / 60]
}

# TT functions

proc TT_Open {host port} {
  set tth [socket $host $port]
  fconfigure $tth -translation binary
  return $tth
}

proc TT_ISA {tth classname objname} {
  set classname [string trim $classname]
  set objname [string trim $objname]
  return [TT_QueryBoolean $tth "ISA $classname $objname"]
}

proc TT_Retrieve {tth picki anci desci mode ptn} {
  set mode [string trim $mode]
  set ptn [string trim $ptn]
  return [TT_QueryAssertionLines $tth "Retrieve $picki $anci $desci $mode $ptn"]
}

proc TT_Tag {tth feat text} {
  set text [TT_ElimNewlines $text]
  set feat [string trim $feat]
  return [TT_QueryPNodes $tth "Tag $feat $text "]
}

proc TT_Close {tth} {
  close $tth
}

# Internal functions

proc TT_ElimNewlines {s} {
  regsub -all "\n" $s " " r
  return $r
}

proc TT_QueryAssertionLines {tth cmd} {
  puts -nonewline $tth "$cmd\n"
  flush $tth
  set lines [TT_Readlines $tth]
  return $lines
}

proc TT_QueryPNodes {tth cmd} {
  puts -nonewline $tth "$cmd\n"
  flush $tth
  set lines [TT_Readlines $tth]
  return [TT_TokenizeMany $lines ":"]
}

proc TT_QueryBoolean {tth cmd} {
  puts -nonewline $tth "$cmd\n"
  flush $tth
  return [TT_Readline $tth]
}

proc TT_Tokenize {s delim} {
  set len [string length $s]
  set frompos 0
  set pos 0
  set r {}
  while {$pos < $len} {
    set c [string index $s $pos]
    if {$c == "\""} {
      set pos [expr $pos + 1]
      while {$pos < $len} {
        set c [string index $s $pos]
        if {$c == "\""} {
          break
        }
        set pos [expr $pos + 1]
      }
      lappend r [string range $s [expr $frompos + 1] [expr $pos - 1]]
      set pos [expr $pos + 2]
      set frompos $pos
    } else {
      if {$c == $delim} {
        lappend r [string range $s $frompos [expr $pos - 1]]
        set pos [expr $pos + 1]
        set frompos $pos
      } else {
        set pos [expr $pos + 1]
      }
    }
  }
  if {$pos > $frompos} {
    lappend r [string range $s $frompos [expr $pos - 1]]
  }
  return $r
}

proc TT_TokenizeMany {lines delim} {
  set r {}
  foreach {line} $lines {
    lappend r [TT_Tokenize $line $delim]
  }
  return $r
}

proc TT_Readline {tth} {
  return [gets $tth]
}

proc TT_Readlines {tth} {
  set r {}
  while {1} {
    set line [TT_Readline $tth]
    if {$line=="."} {
      break
    }
    lappend r $line
  }
  return $r
}

# End of file.
