using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Linq;

namespace StahovaciServisa.stahovace
{
    public class CnbKurzy : IDataProvider
    {


        private const string CnbXmlUrl =
            "https://www.cnb.cz/cs/financni-trhy/devizovy-trh/kurzy-devizoveho-trhu/kurzy-devizoveho-trhu/denni_kurz.xml";


      


        public CnbKurzy()
        {
       
        }


        public string Nazev => "ČNB kurzovní lístek";



        public  XElement ZiskejData()
        {
            HttpClient httpClient = new HttpClient();

            var xml = httpClient.GetStringAsync(CnbXmlUrl).GetAwaiter().GetResult();

       


           

            var dokument = XDocument.Parse(xml);


            return dokument.Root!;
        }
    }
}
