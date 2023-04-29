# On the naming conventions

Many of the naming conventions in the code are inspired by the naming schemes of GLFW and Vulkan. Thus, as a general rule of thumb, the following are true:
- All entities are prefixed with `dfl` in any form, specific to the entity type:
-- Functions are prefixed with `dfl` (All letters lowercase)
-- Types are prefixed with `Dfl` (First letter uppercase)
-- Enum elements and number macros are prefixed with `DFL_` (All letters uppercase)

For types, the general convention is `Dfl` + [phrase that ends in a noun]. CamelCase is used.

For functions, the general convention is `dfl` + [verb phrase]. If the function is meant to modify the properties of an object (or create or destroy it),
then the noun that describes that object will be **the first** to appear in the name of the function (after the `dfl` prefix). Examples:
- `dflWindowCreate` - Creates a new window
- `dflWindowReshape` - Reshapes an existing window
- `dflWindowChangeIcon` - Changes the icon of an existing window
Internal functions (functions that are not meant to be used by the user) will be suffixed with `HIDN` (Hidden).
Callback types will be suffixed with `CLBK` (Callback).
Getter or setter functions will be suffixed with either `Get` or `Set` following the variable to get or set.
CamelCase is used for functions.

# On other conventions

- Functions that modify (or destroy) an object (or are its getter/setter functions) will always have as their **last** parameter a pointer to such an object.
- Functions that create an object will always return a pointer to such an object.
- Typedef is used *only* for function pointers. For all other constructs, typedef is not used, unless absolutely necessary.
