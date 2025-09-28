#.\FixJpgSorting.ps1 -TargetDirectory "D:\path\to\your\directory"
param(
    [Parameter(Mandatory=$true, HelpMessage="Enter target directory path")]
    [string]$TargetDirectory
)

# 验证目录是否存在
if (-not (Test-Path -Path $TargetDirectory -PathType Container)) {
    Write-Host "Error: Directory does not exist - $TargetDirectory" -ForegroundColor Red
    exit 1
}

# 获取所有JPG文件
$jpgFiles = Get-ChildItem -Path $TargetDirectory -Filter *.jpg -File

if (-not $jpgFiles) {
    Write-Host "No JPG files found in directory" -ForegroundColor Yellow
    exit 0
}

# 查找最大数字以确定需要的位数
$maxNumber = 0
foreach ($file in $jpgFiles) {
    if ($file.BaseName -match '^\d+$') {
        $num = [int]$file.BaseName
        if ($num -gt $maxNumber) {
            $maxNumber = $num
        }
    }
}

# 计算需要的位数（至少3位）
$digits = [math]::Max(3, $maxNumber.ToString().Length)
Write-Host "Processing directory: $TargetDirectory"
Write-Host "Found $($jpgFiles.Count) JPG files"
Write-Host "Using $digits-digit format for consistent sorting`n"

$renamedCount = 0
$skippedCount = 0

# 先处理非数字文件名的文件
$nonNumericFiles = $jpgFiles | Where-Object { $_.BaseName -notmatch '^\d+$' }
foreach ($file in $nonNumericFiles) {
    Write-Host "Skipped $($file.Name) (non-numeric file name)" -ForegroundColor DarkYellow
    $skippedCount++
}

# 处理数字文件名的文件（按数字降序排序避免覆盖）
$numericFiles = $jpgFiles | Where-Object { $_.BaseName -match '^\d+$' } | 
                Sort-Object { [int]$_.BaseName } -Descending

foreach ($file in $numericFiles) {
    $originalName = $file.Name
    $number = [int]$file.BaseName
    
    # 格式化为固定位数（前面补零）
    $newName = "{0:D$digits}.jpg" -f $number
    $newPath = Join-Path -Path $file.DirectoryName -ChildPath $newName
    
    # 如果新名称与当前名称相同则跳过
    if ($originalName -eq $newName) {
        Write-Host "Skipped $originalName (already in correct format)" -ForegroundColor DarkYellow
        $skippedCount++
        continue
    }
    
    # 检查目标文件名是否已存在
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

# 输出摘要
Write-Host "`nOperation completed!"
Write-Host "Successfully renamed: $renamedCount files"
Write-Host "Skipped: $skippedCount files"
Write-Host "All files are now in $digits-digit format for proper sorting"