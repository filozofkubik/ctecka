using System;
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
                byte[] buffer = new byte[1024];
                int pocetBytu = stream.Read(buffer, 0, buffer.Length);

                string odpoved = Encoding.UTF8.GetString(buffer, 0, pocetBytu);
                Console.WriteLine($"Odpověď od serveru: {odpoved}");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Chyba při komunikaci se serverem: {ex.Message}");
        }

        try
        {
            Console.WriteLine("Připojuji se k serveru...");
            using (TcpClient client = new TcpClient(ipAdresa, port))
            using (NetworkStream stream = client.GetStream())
            {
                // Zde můžete změnit požadavek na "karta1", "karta2" nebo cokoliv jiného
                string pozadavek = "obnov.txt";

                // Převedení zprávy na bajty
                byte[] dataOdeslat = Encoding.UTF8.GetBytes(pozadavek);

                // Odeslání požadavku na server
                stream.Write(dataOdeslat, 0, dataOdeslat.Length);
                Console.WriteLine($"Odesláno na server: {pozadavek}");

                // Příjem odpovědi ze serveru
                byte[] buffer = new byte[1024];
                int pocetBytu = stream.Read(buffer, 0, buffer.Length);
                byte[] skutecnaData = new byte[pocetBytu];

                // 2. Zkopírujeme platná data z bufferu do tohoto nového pole
                Array.Copy(buffer, skutecnaData, pocetBytu);

                File.WriteAllBytes(adresarpameti + @"\obnov.txt", skutecnaData);
                Console.WriteLine($"Odpověď od serveru: ukladam soubor obnov.txt");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Chyba při komunikaci se serverem: {ex.Message}");
        }

        try
        {
            Console.WriteLine("Připojuji se k serveru...");
            using (TcpClient client = new TcpClient(ipAdresa, port))
            using (NetworkStream stream = client.GetStream())
            {
                // Zde můžete změnit požadavek na "karta1", "karta2" nebo cokoliv jiného
                string pozadavek = "stranka.txt";

                // Převedení zprávy na bajty
                byte[] dataOdeslat = Encoding.UTF8.GetBytes(pozadavek);

                // Odeslání požadavku na server
                stream.Write(dataOdeslat, 0, dataOdeslat.Length);
                Console.WriteLine($"Odesláno na server: {pozadavek}");

                // Příjem odpovědi ze serveru
                byte[] buffer = new byte[1024];
                int pocetBytu = stream.Read(buffer, 0, buffer.Length);
                byte[] skutecnaData = new byte[pocetBytu];

                // 2. Zkopírujeme platná data z bufferu do tohoto nového pole
                Array.Copy(buffer, skutecnaData, pocetBytu);

                File.WriteAllBytes(adresarpameti + @"\stranka.txt", skutecnaData);
                Console.WriteLine($"Odpověď od serveru: ukladam soubor stranka.txt");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Chyba při komunikaci se serverem: {ex.Message}");
        }

        Console.WriteLine("\nStiskněte libovolnou klávesu pro ukončení...");
        Console.ReadKey();
    }
}