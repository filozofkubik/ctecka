using System;
using System.Net.Sockets;
using System.Text;

class TestClient
{
    static void Main()
    {
        // IP adresa serveru (pokud běží na stejném počítači, použijte "127.5000" -> "127.0.0.1")
        string ipAdresa = "127.0.0.1";
        int port = 5000;

        try
        {
            Console.WriteLine("Připojuji se k serveru...");
            using (TcpClient client = new TcpClient(ipAdresa, port))
            using (NetworkStream stream = client.GetStream())
            {
                // Zde můžete změnit požadavek na "karta1", "karta2" nebo cokoliv jiného
                string pozadavek = "karta1";

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

        Console.WriteLine("\nStiskněte libovolnou klávesu pro ukončení...");
        Console.ReadKey();
    }
}