<x0> ::= "<b:1>" <x0><x1> | "<a:2>" <x1><x2> | "<>";
<x1> ::= "<a:2>" <x2> | "<c:5,d:1>" <x3> | "<>";
<x2> ::= "<d:2,a:1>" <x2><x3> | "<b:3>" <x2><x2> | "<a:2>";
<x3> ::= "<a:3,c:1>" <x2><x2> | "<c:2,b:2>" <x3><x3> | "<>";
<x4> ::= "<a:1,b:2>" <x2><x5> | "<a:2,b:3>" <x1><x4> | "<>";
<x5> ::= "<a:1,b:1>" <x2><x2> | "<d:1,c:1>" <x1><x1> | "<>";