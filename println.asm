println proc  ;print what is in ax
    push ax
    push bx
    push cx
    push dx
    push si
    lea si,__number__
    mov bx,10
    add si,4
    cmp ax,0
    jnge negate
print:
    xor dx,dx
    div bx
    mov [si],dl
    add [si],'0'
    dec si
    cmp ax,0
    jne print
    inc si
    lea dx,si
    mov ah,9
    int 21h
    mov ah,2
    mov dl,0DH
    int 21h
    mov ah,2
    mov dl,0AH
    int 21h
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret
negate:
    push ax
    mov ah,2
    mov dl,'-'
    int 21h
    pop ax
    neg ax
    jmp print
println endp