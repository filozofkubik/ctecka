
using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using static System.Runtime.InteropServices.JavaScript.JSType;

class Program
{
    private static byte[] nactidatazesouboru  (string jmenosouboruu)
    {
        return  File.ReadAllBytes(jmenosouboruu);
    }
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
                
                Console.WriteLine("pozadavekobnov");
                data = nactidatazesouboru(cestaKAdresari + "obnov.txt");
            }

            else if (pozadavek == "stranka.txt")
            {
                Console.WriteLine("pozadavekstranka");
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
            else if (pozadavek.Contains("file"))//tvarparametru file_nazevsouboru
            {string nazevsouboru = pozadavek.Substring(5); // Odstranění prefixu "file_"
                Console.WriteLine("priselpozadaveknasoubor");
                odpoved = "pozadavekdpoved";
                data = nactidatazesouboru(cestaKAdresari + nazevsouboru);
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