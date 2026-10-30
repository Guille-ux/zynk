if exists("b:current_syntax")
  finish
endif

syntax keyword zynkKeyword let fn class echo if else return while fi break del for do done
syntax keyword zynkBoolean true false
syntax keyword zynkBuiltin min max push pop len Array input read write index getb newb setb chr ord arg float str clock system include import 

syntax match zynkComment "#.*$"
syntax match zynkNumber "\v<\d+(\.\d+)?>"

syntax match zynkFunctionName "\v<fn>\s+\zs\w+"
syntax match zynkClassName "\v<class>\s+\zs\w+"
syntax match zynkVariableName "\v<let>\s+\zs\w+"

syntax region zynkString start='"' end='"' skip='\\"'

" MAGIA POTAGIA
highlight default link zynkKeyword Keyword
highlight default link zynkBoolean Boolean
highlight default link zynkBuiltin Function
highlight default link zynkComment Comment
highlight default link zynkNumber Number
highlight default link zynkString String

" no se 
"highlight default link zynkFunctionName Function
"highlight default link zynkClassName Type
"highlight default link zynkVariableName Identifier

highlight zynkFunctionName cterm=bold gui=bold guifg=#89b4fa ctermfg=12
highlight zynkClassName cterm=bold gui=bold guifg=#f9e2af ctermfg=11
highlight zynkVariableName guifg=#f38ba8 ctermfg=9


let b:current_syntax = "zynk"
