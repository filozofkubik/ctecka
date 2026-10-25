
using System;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using static System.Runtime.InteropServices.JavaScript.JSType;

class Program
{
    private static byte[] nactidatazesouboru  (string jmenosouboruu)
    {
        return  File.ReadAllBytes(jmenosouboruu);
    }
    private static void VypisDostupneIPAdresy(int port)
    {
        Console.WriteLine("Server je dostupný na těchto IP adresách:");

        // Procházení aktivních síťových rozhraní
        foreach (var networkInterface in NetworkInterface.GetAllNetworkInterfaces())
        {
            // Filtrujeme pouze aktivní (zapnutá) rozhraní, která nejsou typu Loopback (127.0.0.1)
            if (networkInterface.OperationalStatus == OperationalStatus.Up &&
                networkInterface.NetworkInterfaceType != NetworkInterfaceType.Loopback)
            {
                foreach (var ipProperty in networkInterface.GetIPProperties().UnicastAddresses)
                {
                    // Zajímá nás pouze IPv4 adresa
                    if (ipProperty.Address.AddressFamily == AddressFamily.InterNetwork)
                    {
                        Console.WriteLine($"  -> {ipProperty.Address} (Port: {port})");
                    }
                }
            }
        }
    }
    static void Main()
    {
        int port = 5000; // Změňte na požadované číslo portu
        VypisDostupneIPAdresy(port);
        TcpListener server = new TcpListener(IPAddress.Any, port);
        byte[] data;
        server.Start();

        Console.WriteLine("Server spuštěn.");
        //TODO: načíst konfigurace; adresář; číslo portu; ...
        string cestaKAdresari = @"c:\ctecka\com_ctecka\"; // Změňte na cestu k adresáři s obrázky

        while (true)
        {
            TcpClient client = server.AcceptTcpClient();

            Console.WriteLine("Klient připojen.");

            NetworkStream stream = client.GetStream();

            byte[] buffer = new byte[10240000];
            int pocetBytu = stream.Read(buffer, 0, buffer.Length);

            string pozadavek = Encoding.UTF8.GetString(buffer, 0, pocetBytu);

            string odpoved;

            if (pozadavek == "obnov.txt")
            {

                Console.WriteLine("pozadavekobnov");
                data = nactidatazesouboru(cestaKAdresari + "obnov.txt");
            }

            else if (pozadavek == "stranka.txt")
            {
                Console.WriteLine("pozadavekstranka xxxxxx");
                data = nactidatazesouboru(cestaKAdresari + "stranka.txt");
            }
            else if (pozadavek == "seznamobrazu.txt")
            {
                Console.WriteLine("pozadavekseznamobrazu");

                string[] files = Directory.GetFiles(cestaKAdresari, "obraz*.*");
                odpoved = "";
                for (int i = 0; i < files.Length; i++) { odpoved += Path.GetFileName(files[i]) + ";"; }
                odpoved = odpoved.TrimEnd(';');
                data = Encoding.UTF8.GetBytes(odpoved);

            }
            else if (pozadavek.Substring(0, 4) == "echo")
            {
                data = Encoding.UTF8.GetBytes("Echo: " + pozadavek.Substring(4));
                Console.WriteLine("pozadavek echo: " + pozadavek.Substring(4));
            }
            else if (pozadavek.Substring(0, 3) == "log")
            {
                data = Encoding.UTF8.GetBytes(" ");
                Console.WriteLine("pozadavek log: " + pozadavek.Substring(3));
            }
            else if (pozadavek.Contains("file"))//tvarparametru file_nazevsouboru
            {
                string nazevsouboru = pozadavek.Substring(5); // Odstranění prefixu "file_"
                Console.WriteLine("priselpozadaveknasoubor");
                odpoved = "pozadavekdpoved";
                data = nactidatazesouboru(cestaKAdresari + nazevsouboru);
            }
            else
            {
                odpoved = "Nenalezeno";

                data = Encoding.UTF8.GetBytes(odpoved);
                Console.WriteLine("pozadaveknenalezeno");
            }

             

            stream.Write(data, 0, data.Length);

            client.Close();
        }
    }
}