# Quartz programming language

![](https://raw.githubusercontent.com/deltegui/quartz/refs/heads/master/logo/quartz.png "Quartz")

# Notas sobre el estilo de código

### ¿Cuándo usar const T*, T* const y const T* const?
- Si no quieres que el valor al que apunta el puntero sea cambiado, usa const T*. Este va a ser el más habitual.
- En los casos en el que un puntero sea equivalente al 'this' de un lenguaje orientado a objetos o sea un parámetro de salida, usa T* const. Esto es porque en ninguno de estos casos tiene sentido que se modifique el puntero.
- Usa const T* const cuando ocurran las dos condiciones de arriba a la vez.

### Nombrado

- Todas las funciones deben llevar delante de su nombre el nombre del módulo al que pertenecen, exceptuando las funciones create, free e init que lo llevan detrás.
  Por ejemplo expr_add(...) o create_binary_expr(...)
- Usa init_<module>(T* t) para inicializar una estructura.
- Usa T create_<module>() para crear una estructura reservando memoria para ella.
- Usa free_<module>(T* t) para liberar la memoria de esa estructura.
- Usa mark_<module>(T* t) para marcar la memoria de esa estructura.

