using System;
using System.Xml.Linq;

public interface IDataProvider
{

    string Nazev { get; }



    Task<XElement> ZiskejDataAsync(CancellationToken stoppingToken);
}
