# Common parameters
$apiUrl = "https://open.bigmodel.cn/api/paas/v4/chat/completions"
$token = "eyJhbGciOiJIUzI1NiIsInNpZ25fdHlwZSI6IlNJR04ifQ.eyJhcGlfa2V5IjoiN2ZmMjI5YWQzZWQyNDBlNGFmNThkMzk3YzU1ZTNkM2EiLCJleHAiOjE3NTMyNjQzNTUwMDAsInRpbWVzdGFtcCI6MTc1MzI2MDc1NTAwMH0.bn5jbDj8FEUzZaak9XjBVXr0UnXjWbaOViv1YRxSd8g"
$headers = @{
    "Authorization" = "Bearer $token"
    "Content-Type" = "application/json"
    "Accept" = "application/json"
    "User-Agent" = "ESP32-Zhipu-Client/1.0"
}

# =============== Step 1: Initial Request ===============
Write-Host "`n===== Sending Initial Request =====" -ForegroundColor Cyan
$initialBody = @{
    model = "glm-4-flash-250414"
    max_tokens = 16384
    temperature = 0.5
    top_p = 0.5
    messages = @(
        @{
            role = "system"
            content = "You are an AI assistant running on a smart device. You can perform device operations or get real-time information through tool calls. Choose the appropriate tool based on the user's question."
        },
        @{
            role = "user"
            content = "Please set the volume to 50%"
        }
    )
    tools = @(
        @{
            type = "function"
            function = @{
                name = "baidu_search"
                description = "Search the internet using Baidu search engine"
                parameters = @{
                    type = "object"
                    properties = @{
                        query = @{
                            type = "string"
                            description = "Search keywords"
                        }
                    }
                    required = @("query")
                }
            }
        },
        @{
            type = "function"
            function = @{
                name = "adjust_volume"
                description = "Adjust device volume locally"
                parameters = @{
                    type = "object"
                    properties = @{
                        volume_level = @{
                            type = "integer"
                            description = "Volume level (0-100)"
                            minimum = 0
                            maximum = 100
                        }
                    }
                    required = @("volume_level")
                }
            }
        }
    )
    tool_choice = "auto"
} | ConvertTo-Json -Depth 10

try {
    $response = Invoke-RestMethod -Uri $apiUrl -Method Post -Headers $headers -Body $initialBody
    Write-Host "Initial request successful!" -ForegroundColor Green
    
    # Extract tool call information
    $toolCall = $response.choices[0].message.tool_calls[0]
    $toolId = $toolCall.id
    $toolName = $toolCall.function.name
    $toolArgs = $toolCall.function.arguments | ConvertFrom-Json
    
    Write-Host "Tool call ID: $toolId"
    Write-Host "Tool name: $toolName"
    Write-Host "Arguments: $($toolArgs | ConvertTo-Json)"
}
catch {
    Write-Host "Initial request failed: $($_.Exception.Message)" -ForegroundColor Red
    exit
}

# =============== Step 2: Execute Local Operation ===============
Write-Host "`n===== Executing Local Operation =====" -ForegroundColor Cyan
if ($toolName -eq "adjust_volume") {
    $volume = $toolArgs.volume_level
    Write-Host "Setting volume to $volume%..." -ForegroundColor Yellow
    # Simulate actual device operation
    Start-Sleep -Seconds 1
    Write-Host "Volume set to $volume% successfully" -ForegroundColor Green
    $toolResult = "Volume set to ${volume}%"
}
else {
    Write-Host "Unsupported tool call: $toolName" -ForegroundColor Red
    exit
}

# =============== Step 3: Submit Tool Result ===============
Write-Host "`n===== Submitting Tool Result =====" -ForegroundColor Cyan
$finalBody = @{
    model = "glm-4-flash-250414"
    max_tokens = 16384
    temperature = 0.5
    top_p = 0.5
    messages = @(
        @{
            role = "system"
            content = "You are an AI assistant running on a smart device. You can perform device operations or get real-time information through tool calls. Choose the appropriate tool based on the user's question."
        },
        @{
            role = "user"
            content = "Please set the volume to 50%"
        },
        @{
            role = "assistant"
            tool_calls = @(
                @{
                    id = $toolId
                    type = "function"
                    function = @{
                        name = $toolName
                        arguments = $toolArgs | ConvertTo-Json -Compress
                    }
                }
            )
        },
        @{
            role = "tool"
            tool_call_id = $toolId
            content = $toolResult
        }
    )
} | ConvertTo-Json -Depth 10

try {
    $finalResponse = Invoke-RestMethod -Uri $apiUrl -Method Post -Headers $headers -Body $finalBody
    Write-Host "Tool result submitted successfully!" -ForegroundColor Green
    Write-Host "AI Response: $($finalResponse.choices[0].message.content)"
}
catch {
    Write-Host "Tool result submission failed: $($_.Exception.Message)" -ForegroundColor Red
    
    # Error repair suggestions
    if ($_.Exception.Response.StatusCode -eq 400) {
        Write-Host "`n===== 400 Error Repair Suggestions =====" -ForegroundColor Yellow
        Write-Host "1. Check if JWT token is expired (current time: $(Get-Date))"
        Write-Host "2. Verify tool call ID format: $toolId"
        Write-Host "3. Ensure tool parameters meet specifications:"
        Write-Host "   - volume_level should be integer (0-100)"
        Write-Host "   - Current arguments: $($toolArgs | ConvertTo-Json)"
        Write-Host "4. Confirm if '$toolName' tool is registered on the platform"
        
        # Show detailed error response
        $errorStream = $_.Exception.Response.GetResponseStream()
        $reader = New-Object System.IO.StreamReader($errorStream)
        $errorResponse = $reader.ReadToEnd()
        Write-Host "`nError details: $errorResponse" -ForegroundColor Red
    }
}