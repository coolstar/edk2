set "var=%cd%"
rmdir /s /q .\Build
copy ..\_GOP\vbt-c710.bin CorebootPayloadPkg\vbt.bin
copy ..\_GOP\IntelIvbGopDriver_1030.efi CorebootPayloadPkg\IntelGopDriver.efi
build -a IA32 -a X64 -p CorebootPayloadPkg\CorebootPayloadPkgIa32X64.dsc -b RELEASE -t VS2012x86 -n 4
copy .\Build\CorebootPayloadPkgX64\RELEASE_VS2012x86\FV\UEFIPAYLOAD.fd z:\firmware\tianocore\UEFIPAYLOAD-ivb-book.fd