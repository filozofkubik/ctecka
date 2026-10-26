param(
    [Parameter(Mandatory = $true)]
    [string]$Ip,

    [int]$Port = 5500
)

# Posle prikaz "log" na logovaci port 5500.
$client = [System.Net.Sockets.TcpClient]::new()
$memory = [System.IO.MemoryStream]::new()

try {
    $client.Connect($Ip, $Port)
    $stream = $client.GetStream()
    $request = [System.Text.Encoding]::UTF8.GetBytes("log`n")
    $stream.Write($request, 0, $request.Length)

    $buffer = [byte[]]::new(1024)
    while (($count = $stream.Read($buffer, 0, $buffer.Length)) -gt 0) {
        $memory.Write($buffer, 0, $count)
    }

    [Console]::Write([System.Text.Encoding]::UTF8.GetString($memory.ToArray()))
}
finally {
    $memory.Dispose()
    $client.Dispose()
}
