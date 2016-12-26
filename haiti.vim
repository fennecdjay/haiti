function! HaitiFindBuffer(buffer_name)
  for bufNr in range(1, bufnr('$'))
    if bufname(bufNr) == a:buffer_name
      return bufNr
    endif
  endfor
  return 'NONE'
endf

function! HaitiFindFile(line)
  let filename = get(split(getline(a:line), '{'), 1, 'NONE')
  if filename == 'NONE'
    if a:line == 1 " should not happend
      return 'NONE'
    endif
    return HaitiFindFile(a:line - 1)
  endif
  return get(split(getline(a:line), '}'), 0, 'NONE')[2:]
endf


function! HaitiFindLine(line)
  let linenumber = get(split(getline(a:line), ':'), 0, 'NONE')
  if linenumber == 'NONE'
    if a:line == 1 " should not happend
      return 'NONE'
    endif
    return HaitiFindLine(a:line - 1)
  endif
  return linenumber
endf

function! HaitiGo()
  let f = HaitiFindFile(line('.'))
  let l = HaitiFindLine(line('.'))
  "let dir = pwd
  echo f
  sleep 1
  echo substitute(f, '/home/djay/src/git/haiti/', '', 'g')
  sleep 1  
  let b = bufnr(substitute(f, '/home/djay/src/git/haiti/', '', 'g'))
  "let b = HaitiFindBuffer(f)
  "echo b
  "sleep 1
  if f != 'NONE'
    if l != 'NONE'
      if b != -1
        execute 'e ' b
        " goto line
      else
        q!
        execute 'sv +' . l . " " . f
        normal! zv
      endif
    endif
  endif
endf

function! HaitiFile(tag)
  let g:HaitiFilename = expand('%:p')
  let haiti_text = split(system('./haiti ' . expand('%:p') . ' ' . a:tag), '\n')
  let bufNr = HaitiFindBuffer('Haiti') 
  if bufNr == 'NONE'
    new Haiti
  else
    execute 'buf ' bufNr    
  endif
    "execute 'file Haiti:' . g:HaitiFilename
  call append(0, haiti_text)
  " syntax
  syn region HaitiTag  start="\[" end="\]"
  syn region HaitiHead start="{" end="}"
  syn match HaitiColon ':'
  syn match HaitiLine "^\v<\d+>"
  " highligth
  hi HaitiTag   ctermfg=yellow
  hi HaitiHead  ctermfg=green
  hi HaitiColon ctermfg=red
  hi HaitiLine  ctermfg=blue
  " shortcuts
  nmap <silent><buffer> <CR> :call HaitiGo()<CR>
  nmap  <silent><buffer> q    :q!<CR>
  "set nomodifiable
  unlet g:HaitiFilename
endf
" [todo] check in vim
