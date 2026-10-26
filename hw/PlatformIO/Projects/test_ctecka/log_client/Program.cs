using System.Net.Sockets;
using System.Text;

// Pouziti: dotnet run -- <IP_ctecky> [port]
if (args.Length < 1 || args.Length > 2)
{
    Console.Error.WriteLine("Pouziti: dotnet run -- <IP_ctecky> [port]");
    return 1;
}

var host = args[0];
var port = 5500;

if (args.Length == 2 &&
    (!int.TryParse(args[1], out port) || port is < 1 or > 65535))
{
    Console.Error.WriteLine("Port musi byt cislo od 1 do 65535.");
    return 1;
}

using var client = new TcpClient();
client.Connect(host, port);

using var stream = client.GetStream();
var request = Encoding.UTF8.GetBytes("log\n");
stream.Write(request);

using var response = new MemoryStream();
var buffer = new byte[1024];
int bytesRead;

// Server ukonci spojeni, jakmile odesle cely soubor s logem.
while ((bytesRead = stream.Read(buffer, 0, buffer.Length)) > 0)
{
    response.Write(buffer, 0, bytesRead);
}

Console.Write(Encoding.UTF8.GetString(response.ToArray()));
return 0;
