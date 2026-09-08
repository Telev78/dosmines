Add-Type -AssemblyName System.Drawing

$bmpPath = "asset.bmp"
$outputPath = "palette_extracted.png"

# Charger l'image BMP
$img = [System.Drawing.Image]::FromFile((Resolve-Path $bmpPath))

try {
    # Créer une image de sortie (16 carrés de 50x50 pixels, plus d'espace en bas pour le quinconce)
    $swatchSize = 50
    $outWidth = $swatchSize * 16
    $outHeight = $swatchSize + 65 # Espace supplémentaire pour le décalage en quinconce
    
    $paletteImg = New-Object System.Drawing.Bitmap $outWidth, $outHeight
    $graphics = [System.Drawing.Graphics]::FromImage($paletteImg)
    $graphics.Clear([System.Drawing.Color]::White)
    
    $fontIndex = New-Object System.Drawing.Font("Arial", 9, [System.Drawing.FontStyle]::Bold)
    $fontHex = New-Object System.Drawing.Font("Arial", 8)
    $textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::Black)
    $format = New-Object System.Drawing.StringFormat
    $format.Alignment = [System.Drawing.StringAlignment]::Center

    # Récupérer la palette du BMP
    $palette = $img.Palette
    $colorCount = $palette.Entries.Length
    
    Write-Host "Nombre de couleurs trouvées dans la palette : $colorCount"

    for ($i = 0; $i -lt [Math]::Min(16, $colorCount); $i++) {
        $color = $palette.Entries[$i]
        $brush = New-Object System.Drawing.SolidBrush($color)
        
        # Dessiner le carré de couleur
        $rect = New-Object System.Drawing.Rectangle ($i * $swatchSize), 0, $swatchSize, $swatchSize
        $graphics.FillRectangle($brush, $rect)
        
        # Dessiner une bordure noire autour du carré
        $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::Black, 1)
        $graphics.DrawRectangle($pen, $rect)
        
        # Formatter l'index et le code Hex
        $hexCode = "#{0:X2}{1:X2}{2:X2}" -f $color.R, $color.G, $color.B
        $centerX = ($i * $swatchSize) + ($swatchSize / 2)
        
        # Alternance haut / bas (quinconce) une fois sur deux ($i % 2)
        if ($i % 2 -eq 0) {
            $posY1 = $swatchSize + 4
            $posY2 = $posY1 + 16
        } else {
            $posY1 = $swatchSize + 26
            $posY2 = $posY1 + 16
        }
        
        # Écrire l'index [i] et le code hexadécimal à la position alternée
        $graphics.DrawString("[$i]", $fontIndex, $textBrush, $centerX, $posY1, $format)
        $graphics.DrawString($hexCode, $fontHex, $textBrush, $centerX, $posY2, $format)
        
        $brush.Dispose()
        $pen.Dispose()
    }

    # Sauvegarder l'image finale
    $paletteImg.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Host "Palette générée avec succès : $outputPath" -ForegroundColor Green

}
finally {
    $graphics.Dispose()
    $paletteImg.Dispose()
    $img.Dispose()
    $fontIndex.Dispose()
    $fontHex.Dispose()
    $textBrush.Dispose()
}
