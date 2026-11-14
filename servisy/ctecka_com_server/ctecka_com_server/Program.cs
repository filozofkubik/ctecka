
using System;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;

class Program
{
    private const int ClientTimeoutMs = 10_000;

    private sealed record KontextKlienta(TcpClient Klient, string CestaKAdresari);

    private static void SpustObsluhuKlienta(object? stav)
    {
        KontextKlienta kontext = (KontextKlienta)stav!;
        ObsluzKlienta(kontext.Klient, kontext.CestaKAdresari);
    }

    private static byte[] nactidatazesouboru(string jmenosouboruu)
    {
        return File.ReadAllBytes(jmenosouboruu);
    }

    private static void VypisDostupneIPAdresy(int port)
    {
        Console.WriteLine("Server je dostupný na těchto IP adresách:");

        foreach (var networkInterface in NetworkInterface.GetAllNetworkInterfaces())
        {
            if (networkInterface.OperationalStatus == OperationalStatus.Up &&
                networkInterface.NetworkInterfaceType != NetworkInterfaceType.Loopback)
            {
                foreach (var ipProperty in networkInterface.GetIPProperties().UnicastAddresses)
                {
                    if (ipProperty.Address.AddressFamily == AddressFamily.InterNetwork)
                    {
                        Console.WriteLine($"  -> {ipProperty.Address} (Port: {port})");
                    }
                }
            }
        }
    }

    private static void ObsluzKlienta(TcpClient client, string cestaKAdresari)
    {
        using (client)
        {
            try
            {
                client.ReceiveTimeout = ClientTimeoutMs;
                client.SendTimeout = ClientTimeoutMs;

                using NetworkStream stream = client.GetStream();
                using StreamReader reader = new(stream, Encoding.UTF8, true, 1024, leaveOpen: true);
                string? pozadavek = reader.ReadLine();
                if (pozadavek is null)
                {
                    Console.WriteLine("Klient ukončil spojení bez požadavku.");
                    return;
                }

                byte[] data;
                if (pozadavek == "obnov.txt")
                {
                    Console.WriteLine("pozadavekobnov");
                    data = nactidatazesouboru(Path.Combine(cestaKAdresari, "obnov.txt"));
                }
                else if (pozadavek == "stranka.txt")
                {
                    Console.WriteLine("pozadavekstranka xxxxxx");
                    data = nactidatazesouboru(Path.Combine(cestaKAdresari, "stranka.txt"));
                }
                else if (pozadavek == "seznamobrazu.txt")
                {
                    Console.WriteLine("pozadavekseznamobrazu");
                    string[] files = Directory.GetFiles(cestaKAdresari, "obraz*.*");
                    string odpoved = string.Join(";", Array.ConvertAll(files, Path.GetFileName));
                    data = Encoding.UTF8.GetBytes(odpoved);
                }
                else if (pozadavek.StartsWith("echo", StringComparison.Ordinal))
                {
                    string text = pozadavek[4..];
                    data = Encoding.UTF8.GetBytes("Echo: " + text);
                    Console.WriteLine("pozadavek echo: " + text);
                }
                else if (pozadavek.StartsWith("log", StringComparison.Ordinal))
                {
                    data = Encoding.UTF8.GetBytes(" ");
                    Console.WriteLine("pozadavek log: " + pozadavek[3..]);
                }
                else if (pozadavek.StartsWith("file_", StringComparison.Ordinal))
                {
                    string nazevsouboru = pozadavek[5..];
                    if (string.IsNullOrWhiteSpace(nazevsouboru) || Path.GetFileName(nazevsouboru) != nazevsouboru)
                    {
                        throw new InvalidDataException("Neplatný název souboru v požadavku.");
                    }

                    Console.WriteLine("priselpozadaveknasoubor");
                    data = nactidatazesouboru(Path.Combine(cestaKAdresari, nazevsouboru));
                }
                else
                {
                    data = Encoding.UTF8.GetBytes("Nenalezeno");
                    Console.WriteLine("pozadaveknenalezeno");
                }

                stream.Write(data, 0, data.Length);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Chyba při obsluze klienta: {ex.Message}");
            }
        }
    }

    static void Main()
    {
        int port = 5000;
        VypisDostupneIPAdresy(port);
        TcpListener server = new(IPAddress.Any, port);
        server.Start();

        Console.WriteLine("Server spuštěn.");
        string cestaKAdresari = @"c:\ctecka\com_ctecka\";

        while (true)
        {
            TcpClient client = server.AcceptTcpClient();
            Console.WriteLine("Klient připojen.");
            KontextKlienta kontext = new(client, cestaKAdresari);
            _ = Task.Factory.StartNew(SpustObsluhuKlienta, kontext);
        }
    }
}