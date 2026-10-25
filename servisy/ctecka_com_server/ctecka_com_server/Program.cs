
using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;

class Program
{
    static void Main()
    {
        TcpListener server = new TcpListener(IPAddress.Any, 5000);
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

            byte[] buffer = new byte[1024];
            int pocetBytu = stream.Read(buffer, 0, buffer.Length);

            string pozadavek = Encoding.UTF8.GetString(buffer, 0, pocetBytu);

            string odpoved;

            if (pozadavek == "obnov.txt")
            {
                byte[] dataToSend = null;
                Console.WriteLine("pozadavekobnov");
                data = File.ReadAllBytes(cestaKAdresari + "obnov.txt");
            }

            else if (pozadavek == "stranka.txt")
            {
                Console.WriteLine("pozadavekstranka");
                odpoved = "Drak;20";
                data = Encoding.UTF8.GetBytes(odpoved);
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
            else if (pozadavek.Contains("file"))
            {
                Console.WriteLine("priselpozadaveknasoubor");
                odpoved = "pozadavekdpoved";
                data = Encoding.UTF8.GetBytes(odpoved);
            }
            else
            {
                odpoved = "Nenalezeno";
                data = Encoding.UTF8.GetBytes(odpoved);
            }

             

            stream.Write(data, 0, data.Length);

            client.Close();
        }
    }
}