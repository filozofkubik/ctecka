using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

class TestClient    
{
    private static void smazadresar(string jmenoadresare)
    {
        if (Directory.Exists(jmenoadresare))
        {
            // 1. Smažeme všechny soubory uvnitř
            foreach (string file in Directory.GetFiles( jmenoadresare))
            {
                File.Delete(file);
            }
        }
        else { Console.WriteLine("adresarneexistujee"+ jmenoadresare); }
    }
    private static void ulozsoubor(string pozadavek, string jmenosouboru,string ipAdresa, int port,string adresarpameti)
    {
        try
        {
            Console.WriteLine("Připojuji se k serveru...");
            using (TcpClient client = new TcpClient(ipAdresa, port))
            using (NetworkStream stream = client.GetStream())
            {
                // Zde můžete změnit požadavek na "karta1", "karta2" nebo cokoliv jiného
                

                // Převedení zprávy na bajty
                byte[] dataOdeslat = Encoding.UTF8.GetBytes(pozadavek);

                // Odeslání požadavku na server
                stream.Write(dataOdeslat, 0, dataOdeslat.Length);
                Console.WriteLine($"Odesláno na server: {pozadavek}");

                // Příjem odpovědi ze serveru
                byte[] buffer = new byte[10240000];
                int pocetBytu = stream.Read(buffer, 0, buffer.Length);
                byte[] skutecnaData = new byte[pocetBytu];

                // 2. Zkopírujeme platná data z bufferu do tohoto nového pole
                Array.Copy(buffer, skutecnaData, pocetBytu);

                File.WriteAllBytes(adresarpameti + jmenosouboru, skutecnaData);
                Console.WriteLine($"Odpověď od serveru: ukladam soubor {jmenosouboru}");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Chyba při komunikaci se serverem: {ex.Message}");
        }



    }
    static void Main()
    {
        // IP adresa serveru (pokud běží na stejném počítači, použijte "127.5000" -> "127.0.0.1")
        string ipAdresa = "127.0.0.1";
        int port = 5000;
        string adresarpameti = @"c:\ctecka\com_ctecka_test\pamet\";
        string adresarkarty = @"c:\ctecka\com_ctecka_test\karta\";
        smazadresar(adresarpameti);
        smazadresar(adresarkarty);
        // TODO: načíst konfigurace; adresář; číslo portu; ...
        try
        {
            string[] x;
            Console.WriteLine("Připojuji se k serveru...");
            using (TcpClient client = new TcpClient(ipAdresa, port))
            using (NetworkStream stream = client.GetStream())
            {
                // Zde můžete změnit požadavek na "karta1", "karta2" nebo cokoliv jiného
                string pozadavek = "seznamobrazu.txt";

                // Převedení zprávy na bajty
                byte[] dataOdeslat = Encoding.UTF8.GetBytes(pozadavek);

                // Odeslání požadavku na server
                stream.Write(dataOdeslat, 0, dataOdeslat.Length);
                Console.WriteLine($"Odesláno na server: {pozadavek}");

                // Příjem odpovědi ze serveru
                byte[] buffer = new byte[1024000];
                int pocetBytu = stream.Read(buffer, 0, buffer.Length);

                string odpoved = Encoding.UTF8.GetString(buffer, 0, pocetBytu);
                Console.WriteLine($"Odpověď od serveru: {odpoved}");
                  x =odpoved.Split(";");
            
            }
            foreach (string a in x)
            {
                ulozsoubor("file_"+a, a, ipAdresa, port, adresarkarty);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Chyba při komunikaci se serverem: {ex.Message}");
        }

        
        ulozsoubor("obnov.txt", "obnov.txt", ipAdresa, port, adresarpameti);
        ulozsoubor("stranka.txt", "stranka.txt", ipAdresa, port, adresarpameti);


        Console.WriteLine("\nStiskněte libovolnou klávesu pro ukončení...");
        Console.ReadKey();
    }
}