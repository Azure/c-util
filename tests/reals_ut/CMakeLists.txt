#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName c_util_reals_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

build_test_artifacts(${theseTestsName} ON "tests/c_util" ADDITIONAL_LIBS c_util c_util_reals c_pal)
