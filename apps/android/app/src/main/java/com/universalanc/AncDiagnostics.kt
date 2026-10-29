package com.universalanc
import android.content.Context
import java.io.File
class AncDiagnostics(private val context: Context) {
    fun export(text: String): File {
        val f=File(context.getExternalFilesDir(null),"anc_diagnostics.txt")
        f.writeText("Universal ANC Lab 2.0\nSimulated values are not measured acoustic attenuation.\n\n$text\n")
        return f
    }
}
