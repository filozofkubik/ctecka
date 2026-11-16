using System.Text.Json;
using System.Xml.Linq;
using StahovaciServisa.stahovace.komponety;


namespace StahovaciServisa.stahovace
{
    public class Pocasi : IDataProvider
    {
        private const string ChmuXmlUrl =
            "https://opendata.chmi.cz/meteorology/weather/forecast/now/web_pCRntx_161600.json";

        public Pocasi()
        {
        }

        public string Nazev => "Český hydrometeorologický úřad";

        // Pokud rozhraní IDataProvider vyžaduje Asynchronní metodu (Task), 
        // můžete metodu nechat jako async/Task, ale uvnitř volat HTTP požadavek synchronně.
        public XElement ZiskejData( )
        {
            XElement element = null;
            try { 
          
             HttpClient httpClient = new HttpClient();

            string json = httpClient.GetStringAsync(ChmuXmlUrl).GetAwaiter().GetResult();

            using JsonDocument doc = JsonDocument.Parse(json);

            ReadPocasi readpocasi = new ReadPocasi(doc);
            readpocasi.ZjistiPocasi();

            string vysledekPocasi = readpocasi.VypisPocasi();

            Console.WriteLine(vysledekPocasi);

             element = new XElement("Pocasi",
                new XAttribute("Zdroj", this.Nazev),
                vysledekPocasi
            );
            } catch (Exception e)
            {
                Console.WriteLine(e.Message);

            }
            return element;
        }
    }
}
