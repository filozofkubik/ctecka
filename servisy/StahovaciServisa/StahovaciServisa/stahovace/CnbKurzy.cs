using StahovaciServisa.stahovace.komponety;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Linq;

namespace StahovaciServisa.stahovace
{
    public class CnbKurzy : IDataProvider
    {


        private readonly string CnbXmlUrl;//             "https://www.cnb.cz/cs/financni-trhy/devizovy-trh/kurzy-devizoveho-trhu/kurzy-devizoveho-trhu/denni_kurz.xml";



        private readonly Konfigurace _k;

        public CnbKurzy(Konfigurace konfigurace)
        {
            _k = konfigurace;
            CnbXmlUrl = _k.url;
        }


        public string Nazev => "ČNB kurzovní lístek";



        public XElement ZiskejData()
        {

            try
            {
                Logovani.ZapisDoLogu(UrovenLogovani.Info, _k.urovenLogovani, _k.chybovyadresar, "Stahuji data z ČNB");
                HttpClient httpClient = new HttpClient();

                var xml = httpClient.GetStringAsync(CnbXmlUrl).GetAwaiter().GetResult();

                var dokument = XDocument.Parse(xml);

                Logovani.ZapisDoLogu(UrovenLogovani.Info, _k.urovenLogovani, _k.chybovyadresar, "dokončweno stahovaní data z ČNB");
                return dokument.Root!;
            }
            catch (Exception e)
            {
                Logovani.ZapisDoLogu(UrovenLogovani.Chyba, _k.urovenLogovani, _k.chybovyadresar, $"Chyba při stahování dat z ČNB: {e.Message}");
                throw; 
            }
        }
    }
}