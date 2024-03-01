# Coding Rules

## Indentation

Every indentations are 4 whitespaces (U+0020) except for the documents which needs tabs for indentation.
No more and less whitespaces are allowed even for the alignment.

In `switch` block, indentations level is increased both starting of the statement and cases. e.g.:
```c
switch (var) {
    case 1:
        do_something1();
        break;
    case 2:
        do_something2();
        break;
    case 3:
        do_something3();
        break;
    default:
        break;
}
```

## Line Breaks

The column length limit is 100 characters, but below 80 is highly recommended.
Do not exceed 80 characters if you can break the line with the methods listed below:

### In Case of Statements
1.  Break after the assignment operator
    ```c
    very_long_type_name_t very_long_variable_name =
        very_long_expression + extremely_long_expression;
    ```
2.  Break after the condition part of a ternary operator
    ```c
    very_long_type_name_t very_long_variable_name =
        very_long_logical_expression ?
            "very_long_literal" : "another_long_literal";
    ```
3.  Break after the binary operator
    ```c
    very_long_type_name_t very_long_variable_name =
        (very_long_struct_name->very_long_member1 * value1) +
        (very_long_struct_name->very_long_member2 * value2) +
        (very_long_logical_expression ?
            very_long_integer_variable_name1 : very_long_integer_variable_name2);
    ```
4.  Break function call arguments
     ```c
    int some_value = some_function(
        function_argument1,
        function_argument2,
        function_argument3,
        function_argument4);
     ```

### In Case of Function Declaration
1.  Break after the return type
    ```c
    static int
    some_function(int x);
    ```
2.  Break parameters
    ```c
    static int
    some_function(
        int arg1,
        int arg2,
        long_type_name_t arg3,
        ...);
    ```

### In Case of Function Definition


## Spaces

## Braces

## Function & Variable Naming Conventions

## Type Naming Conventions

## Using `goto` Statement

## Macros

## Comments
