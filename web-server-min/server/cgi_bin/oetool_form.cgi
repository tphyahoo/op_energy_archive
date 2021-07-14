#!/bin/bash
echo '<!DOCTYPE html>'
echo '<html>'
echo '<head>'
echo '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">'
echo '<title>oetool rorm </title>'
echo '</head>'
echo '<body style="background-color: rgb( 80, 100, 155); color: rgb(240,240,120)" >'

echo '<h3> Calc OP_ENERGY test </h3>'
  echo '<form method="get">'\
       '<table nowrap>'\
          '<tr><td>idA (64hex): </TD><TD><input type="text" name="idA" value="" ></td></tr>'\
          '<tr><td>idB (64hex): </td><td><input type="text" name="idB" value=""></td>'\
          '</tr></table>'


  echo '<br><input type="submit" value="do_calc">'

echo '</body>'
echo '</html>'

