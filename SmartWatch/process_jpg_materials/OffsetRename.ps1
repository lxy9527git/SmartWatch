#.\OffsetRename.ps1 -TargetDirectory "D:\path\to\240_280_yasuo" -Offset 3
param(
    [Parameter(Mandatory=$true, HelpMessage="Enter target directory path")]
    [string]$TargetDirectory,
    
    [Parameter(Mandatory=$true, HelpMessage="Enter number to add to file names")]
    [int]$Offset
)

# Validate directory exists
if (-not (Test-Path -Path $TargetDirectory -PathType Container)) {
    Write-Host "Error: Directory does not exist - $TargetDirectory" -ForegroundColor Red
    exit 1
}

# Get all JPG files sorted by numeric value
$jpgFiles = Get-ChildItem -Path $TargetDirectory -Filter *.jpg -File | 
            Sort-Object { [int]($_.BaseName -replace '\D') } -Descending

if (-not $jpgFiles) {
    Write-Host "No JPG files found in directory" -ForegroundColor Yellow
    exit 0
}

Write-Host "Processing directory: $TargetDirectory"
Write-Host "Adding offset: $Offset to file numbers`n"
Write-Host "Found $($jpgFiles.Count) JPG files`n"

$renamedCount = 0
$skippedCount = 0

foreach ($file in $jpgFiles) {
    $originalName = $file.Name
    
    # Extract numeric part from file name
    if ($file.BaseName -match '^\d+$') {
        $currentNumber = [int]$file.BaseName
        $newNumber = $currentNumber + $Offset
        $newName = "$newNumber.jpg"
        $newPath = Join-Path -Path $file.DirectoryName -ChildPath $newName
        
        # Check if target filename already exists
        if (Test-Path -LiteralPath $newPath -PathType Leaf) {
            Write-Host "Skipped $originalName -> $newName (file already exists)" -ForegroundColor DarkYellow
            $skippedCount++
        }
        else {
            try {
                Rename-Item -LiteralPath $file.FullName -NewName $newName -ErrorAction Stop
                Write-Host "Renamed: $originalName -> $newName" -ForegroundColor Green
                $renamedCount++
            }
            catch {
                Write-Host "Error: Failed to rename $originalName - $_" -ForegroundColor Red
                $skippedCount++
            }
        }
    }
    else {
        Write-Host "Skipped $originalName (not a numeric file name)" -ForegroundColor DarkYellow
        $skippedCount++
    }
}

# Output summary
Write-Host "`nOperation completed!"
Write-Host "Successfully renamed: $renamedCount files"
Write-Host "Skipped: $skippedCount files"