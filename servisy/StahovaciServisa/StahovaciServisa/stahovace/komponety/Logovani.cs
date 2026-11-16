using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

namespace StahovaciServisa.stahovace.komponety
{
    public enum UrovenLogovani
    {
        Info,
        Chyba

    }
    static class Logovani
    {
        static public void ZapisDoLogu( UrovenLogovani uroven, UrovenLogovani nastavenaUrovenKonfigurace, string adresar,string zprava)
        {
            Console.WriteLine(zprava);
            Debug.Print(zprava);
            try
            {
                if (uroven == UrovenLogovani.Info)
                {
                    System.IO.File.AppendAllText(System.IO.Path.Combine(adresar, "log.txt"), DateTime.Now.ToString() + " " + uroven.ToString() + " " + zprava + Environment.NewLine);
                }

                if (uroven == UrovenLogovani.Chyba)
                {
                    System.IO.File.AppendAllText(System.IO.Path.Combine(adresar, "log.txt"), DateTime.Now.ToString() + " " + uroven.ToString() + " " + zprava + Environment.NewLine);
                    System.IO.File.AppendAllText(System.IO.Path.Combine(adresar, "log.err"), DateTime.Now.ToString() + " " + uroven.ToString() + " " + zprava + Environment.NewLine);
                }
            }
            catch (Exception ex)
            {
                Debug.Print("Chyba při zápisu do logu: " + ex.Message);
                Console.WriteLine("Chyba při zápisu do logu: " + ex.Message);

            }

        }
    }

}