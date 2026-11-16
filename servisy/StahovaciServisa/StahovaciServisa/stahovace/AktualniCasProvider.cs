using StahovaciServisa.stahovace.komponety;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Linq;

namespace StahovaciServisa.stahovace
{
    public class AktualniCasProvider : IDataProvider
    {
        private readonly Konfigurace _k;
        public string Nazev => "Aktuální čas";
        public AktualniCasProvider(Konfigurace konfigurace)
        {   
           _k = konfigurace; ;
        }   

        public XElement ZiskejData()
        {
            try {
                Logovani.ZapisDoLogu(UrovenLogovani.Info, _k.urovenLogovani, _k.chybovyadresar, "Získávám aktuální čas");
                string cas = DateTime.Now.ToString("d.M.yyyy HH:mm:ss");


                var element = new XElement("aktualniCas", cas);

                Logovani.ZapisDoLogu(UrovenLogovani.Info, _k.urovenLogovani, _k.chybovyadresar, "Získávám aktuální čas dokončeno");
                return element;
            }
            catch (Exception e)
            {
                Logovani.ZapisDoLogu(UrovenLogovani.Chyba, _k.urovenLogovani, _k.chybovyadresar, $"Chyba při získávání aktuálního času: {e.Message}");
                throw;
            }
         
        }
    }
}
