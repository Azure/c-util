# PowerShell script to reorder the unit test functions to match the API order
$filePath = "tests\constbuffer_thandle_ut\constbuffer_thandle_ut.c"
$content = Get-Content $filePath -Raw

# Define the section boundaries (approximate line numbers based on current file)
$sections = @{
    "CONSTBUFFER_THANDLE_Create" = @{start=134; end=230}
    "CONSTBUFFER_THANDLE_GetContent" = @{start=231; end=314}
    "CONSTBUFFER_THANDLE_CreateWithMoveMemory" = @{start=315; end=469}
    "CONSTBUFFER_THANDLE_CreateFromBuffer" = @{start=470; end=533}
    "CONSTBUFFER_THANDLE_contain_same" = @{start=534; end=663}
    "CONSTBUFFER_THANDLE_CreateFromOffsetAndSize" = @{start=664; end=916}
    "CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy" = @{start=917; end=1164}
    "CONSTBUFFER_THANDLE_get_serialization_size" = @{start=1165; end=1243}
    "CONSTBUFFER_THANDLE_to_buffer" = @{start=1244; end=1345}
    "CONSTBUFFER_THANDLE_to_fixed_size_buffer" = @{start=1346; end=1471}
    "CONSTBUFFER_THANDLE_from_buffer" = @{start=1472; end=1692}
    "CONSTBUFFER_THANDLE_CreateWithCustomFree" = @{start=1693; end=1866}
    "CONSTBUFFER_THANDLE_CreateWritableHandle" = @{start=1867; end=1917}
    "CONSTBUFFER_THANDLE_GetWritableBuffer" = @{start=1918; end=1950}
    "CONSTBUFFER_THANDLE_SealWritableHandle" = @{start=1951; end=2025}
    "CONSTBUFFER_THANDLE_GetWritableBufferSize" = @{start=2026; end=2058}
}

# Target order based on API declaration order
$targetOrder = @(
    "CONSTBUFFER_THANDLE_Create",
    "CONSTBUFFER_THANDLE_CreateFromBuffer",
    "CONSTBUFFER_THANDLE_CreateWithMoveMemory", 
    "CONSTBUFFER_THANDLE_CreateWithCustomFree",
    "CONSTBUFFER_THANDLE_CreateFromOffsetAndSize",
    "CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy",
    "CONSTBUFFER_THANDLE_GetContent",
    "CONSTBUFFER_THANDLE_contain_same",
    "CONSTBUFFER_THANDLE_get_serialization_size",
    "CONSTBUFFER_THANDLE_to_buffer",
    "CONSTBUFFER_THANDLE_to_fixed_size_buffer",
    "CONSTBUFFER_THANDLE_from_buffer",
    "CONSTBUFFER_THANDLE_CreateWritableHandle",
    "CONSTBUFFER_THANDLE_GetWritableBuffer",
    "CONSTBUFFER_THANDLE_SealWritableHandle",
    "CONSTBUFFER_THANDLE_GetWritableBufferSize"
)

Write-Host "This script would reorder the test sections, but requires careful implementation"
Write-Host "Target order: $($targetOrder -join ', ')"
Write-Host "Current file has $($content.Length) characters"
