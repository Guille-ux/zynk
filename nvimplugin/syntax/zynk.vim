if exists("b:current_syntax")
  finish
endif

" Palabras clave
syntax keyword zynkKeyword let fn class echo if else return while fi break del for do done
syntax keyword zynkBoolean true false

" Funciones nativas y especiales
syntax keyword zynkBuiltin min max push pop len Array input read write index getb newb setb chr ord arg float str clock system include import 

" Comentarios (soporte para #)
syntax match zynkComment "#.*$"

" Números (enteros y decimales)
syntax match zynkNumber "\v<\d+(\.\d+)?>"

" Cadenas de texto
syntax region zynkString start='"' end='"' skip='\\"'

" Asignación de grupos de color
highlight default link zynkKeyword Keyword
highlight default link zynkBoolean Boolean
highlight default link zynkBuiltin Function
highlight default link zynkComment Comment
highlight default link zynkNumber Number
highlight default link zynkString String

let b:current_syntax = "zynk"
