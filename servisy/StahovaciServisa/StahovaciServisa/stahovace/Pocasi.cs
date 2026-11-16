using System.Text.Json;
using System.Xml.Linq;
using StahovaciServisa.stahovace.komponety;


namespace StahovaciServisa.stahovace
{
    public class Pocasi : IDataProvider
    {
        private const string ChmuXmlUrl =
            "https://opendata.chmi.cz/meteorology/weather/forecast/now/web_pCRntx_161600.json";
        private readonly Konfigurace _k;
        public Pocasi(Konfigurace konfigurace)
        {
            _k = konfigurace;
        }

        public string Nazev => "Český hydrometeorologický úřad";

        // Pokud rozhraní IDataProvider vyžaduje Asynchronní metodu (Task), 
        // můžete metodu nechat jako async/Task, ale uvnitř volat HTTP požadavek synchronně.
        public XElement ZiskejData( )
        {
            XElement element = null;
            try {
                Logovani.ZapisDoLogu(UrovenLogovani.Info, _k.urovenLogovani, _k.chybovyadresar, "Stahuji data z ČHMU");

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
                Logovani.ZapisDoLogu(UrovenLogovani.Info, _k.urovenLogovani, _k.chybovyadresar, "Stahuji dokončeno data z ČHMU");
                return element;
            } 
            catch (Exception e)
            {
                Logovani.ZapisDoLogu(UrovenLogovani.Chyba, _k.urovenLogovani, _k.chybovyadresar, $"Chyba při stahování dat z ČHMU: {e.Message}");
                throw; ;

            }
        
        }
    }
}
