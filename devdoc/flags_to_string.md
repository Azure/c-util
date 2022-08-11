# `flags_to_string` requirements

`flags_to_string` is a module that builds (by C macro means) stringification functions for collection of flags. The flags are assumed to be part of a uint32_t (aka DWORD in windows). The flags should be disjointed, that is, no bits should be shared between the flags. 

Here's an example:

Given the collection of flag values and flag names 1, "ONE", 2, "TWO" then `FLAGS_TO_STRING(COLLECTION)(3)` will produce the string "ONE | TWO".

In the exposed API below "X" is the name that is used to decorate the generated functions so that they are all named differently.

## Exposed API

```c
/*introduce a name for function that can be called. The name is based on "X", User code might look like FLAGS_TO_STRING(ISC_REQ)(value)*/
#define FLAGS_TO_STRING(X) MU_C2(FLAGS_TO_STRING_,X)

/*this macro declares a function to be used in a header*/
#define FLAGS_TO_STRING_DECLARE_FUNCTION(X) MOCKABLE_FUNCTION(, char*, FLAGS_TO_STRING(X), uint32_t, argument)

/*this macro defined a function to be used in a .c file. ... is a list of pairs of FLAG_NAME and FLAG_VALUE. Flag name is a string in quotes and FLAG_VALUE is a number*/
#define FLAGS_TO_STRING_DEFINE_FUNCTION(X, ...) \
char* FLAGS_TO_STRING(X)(uint32_t argument)     \
{                                               \
    /*content*/                                 \
}
```

**SRS_FLAGS_TO_STRING_02_001: [** For each of the known flags `FLAGS_TO_STRING(X)` shall define 2 variables: `separator_N` and `value_N` both of them of type `const char*`. N is a decreasing number (all the way to 1) for every of the predefined flags. **]** 

**SRS_FLAGS_TO_STRING_02_002: [** `separator_N` shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. **]**

**SRS_FLAGS_TO_STRING_02_003: [** If the flag is set then `value_N` shall contain the stringification of the flag. **]**

**SRS_FLAGS_TO_STRING_02_004: [** If the flag is not set then `value_N` shall contain "" (empty string). **]**

**SRS_FLAGS_TO_STRING_02_005: [** `FLAGS_TO_STRING(X)` shall reset (set to 0) all the used known flags in `argument`. **]**

**SRS_FLAGS_TO_STRING_02_006: [** If following the reset of the known flags, there are still bits set in 
`argument`, then `FLAGS_TO_STRING(X)` shall prepare a string using the format `"%s%s...%sUNKNOWN_FLAG(%#.8" PRIx32 ")"`, where 
  each pair of %s%s corresponds to a pair of separator_N/value_N;
  the last %s corresponds to either "" or " | " depeding on `argument` not containing or containing any known flags;
  %PRIx32 corresponds to the remaining (unknown) flags; **]**

  Note: the number of `%s` in the string above is equal to the number of known flags.

**SRS_FLAGS_TO_STRING_02_007: [** If following the reset of the known flags, there are no bits set in `argument`, then `FLAGS_TO_STRING(X)` shall prepare a string using the format `"%s%s...%s"`, where each pair of %s%s corresponds to a pair of separator_N/value_N; **]**

  Note: the number of `%s` in the string above is equal to the number of known flags.

**SRS_FLAGS_TO_STRING_02_008: [** `FLAGS_TO_STRING(X)` shall succeed and return a non-`NULL` string. **]**

**SRS_FLAGS_TO_STRING_02_009: [** If there are any failures then `FLAGS_TO_STRING(X)` shall fails and return `NULL`. **]**
