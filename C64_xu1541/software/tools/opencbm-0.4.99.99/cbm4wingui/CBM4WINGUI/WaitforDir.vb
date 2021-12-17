#Region "Copyright"
'-----------------
'GUI4CBM4WIN
'
' Copyright (C) 2004-2005 Leif Bloomquist
' Copyright (C) 2006      Wolfgang 0.4.0
' Copyright (C) 2006      Spiro Trikaliotis
' Copyright (C) 2006-2007 Payton Byrd
'
' This software Is provided 'as-is', without any express or implied
' warranty. In no event will the authors be held liable for any damages
' arising from the use of this software.
'
' Permission is granted to anyone to use this software for any purpose,
' including commercial applications, and to alter it and redistribute it
' freely, subject to the following restrictions:
'
'     1. The origin of this software must not be misrepresented; you must
'        not claim that you wrote the original software. If you use this
'        software in a product, an acknowledgment in the product
'        documentation would be appreciated but is not required.
'
'     2. Altered source versions must be plainly marked as such, and must
'        not be misrepresented as being the original software.
'
'     3. This notice may not be removed or altered from any source
'        distribution.
'
#End Region

Option Strict Off
Option Explicit On
Friend Class Waiting
	Inherits System.Windows.Forms.Form

	
	
    Private Sub Waiting_Load(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles MyBase.Load
    End Sub
	
    Private Sub LEDTimer_Tick(ByVal eventSender As System.Object, ByVal eventArgs As System.EventArgs) Handles LEDTimer.Tick
        If System.Drawing.ColorTranslator.ToOle(LED.BackColor) = System.Drawing.ColorTranslator.ToOle(System.Drawing.Color.Red) Then
            LED.BackColor = System.Drawing.Color.Black
        Else
            LED.BackColor = System.Drawing.Color.Red
        End If
    End Sub
End Class