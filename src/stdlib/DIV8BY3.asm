	SECTION code

DIV8BY3	EXPORT


* Input: B = Unsigned byte to be divided by 3.
* Output: Quotient in B.
* Trashes A.
*
* Source: https://retrocomputing.stackexchange.com/questions/25568 (user3528438)
* int div3(int x) {
*     int a = x>>4;
*     int b = x&0x0f;
*     int y = a + (a<<2);
*     b = a+b;
*     y+=((b<<2)+b+(b>>1))>>4; // use 5.5/16 to avoid overflow, result is the same
*     return y;
* }
*
DIV8BY3
        tfr     b,a
        lsra
        lsra
        lsra
        lsra
        PSHS    a       ; a = x>>4
        lsla
        lsla            ; a<<2
        adda    ,s      ; y = a + (a<<2)
        PSHS    a       ; y
        andb    #$0F    ; b = x&0x0f
        addb    1,s     ; b = a+b

        PSHS    b       ; b
        lslb
        lslb
        PSHS    b       ; b<<2
        ldb     1,s     ; b
        lsrb            ; b>>1
        addb    ,s+     ; (b<<2) + (b>>1)
        addb    ,s+     ; (b<<2) + b + (b>>1)

        lsrb
        lsrb
        lsrb
        lsrb

        addb    ,s+     ; add y

        puls    a,pc    ; discard temp byte, return


	ENDSECTION
