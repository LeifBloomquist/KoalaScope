using KoalaScopeServer;
using System.Net;

public class HttpServer
{
    public int Port = 8064;
    string path = "/ml/koala/";
    private HttpListener _listener = new();

    private FileCollection? _filecollection;
    private Random random = new();

    public void Start(bool local, FileCollection file_collection)
    {
        _filecollection = file_collection;

        if (local)
        {
            Port = 80;
            path = "/test/";
        }

        //_listener.IgnoreWriteExceptions = true;
        _listener.Prefixes.Add("http://+:" + Port.ToString() + path);
        _listener.Start();

        Console.WriteLine("Server listening on " + _listener.Prefixes.ElementAt(0).ToString());

        Receive();
    }

    public void Stop()
    {
        _listener.Stop();
    }

    private void Receive()
    {
        _listener.BeginGetContext(new AsyncCallback(ListenerCallback), _listener);
    }

    private void ListenerCallback(IAsyncResult result)
    {
        if (_listener.IsListening)
        {
            var context = _listener.EndGetContext(result);
            var request = context.Request;

            Console.WriteLine($"Received request [{request.Url}] from [{request.RemoteEndPoint}] with method [{request.HttpMethod}]");

            var response = context.Response;
            response.StatusCode = (int)HttpStatusCode.OK;
            response.ContentType = "application/octet-stream";
            response.SendChunked = true;
            response.KeepAlive = true;

            byte[] response_bytes = [];

            if (request.HttpMethod == HttpMethod.Get.ToString())
            {
                if (request.RawUrl == null) return;
                if (_filecollection == null) return;

                UInt32 num_files = _filecollection.GetFileCount();

                if (request.RawUrl.Equals(path + "count.prg"))
                {
                    byte[] b = BitConverter.GetBytes(num_files);
                    response_bytes = [0x00, 0xC0, b[0], b[1], b[2], b[3]];
                }

                if (request.RawUrl.Equals(path + "random" + Constants.KOA))
                {                    
                    int chosen = random.Next((int)num_files);
                    Console.WriteLine($"Sending random file #[{chosen}] out of [{num_files}] total with filename [{_filecollection.GetFileNames()[chosen]}]");
                    response_bytes = _filecollection.GetFileContents(chosen);
                }

                if (request.RawUrl.Equals(path + "latest" + Constants.KOA))
                {
                    int latest = TimedCounter.CurrentCount;
                    Console.WriteLine($"Sending latest file #[{latest}] out of [{num_files}] total with filename [{_filecollection.GetFileNames()[latest]}]");
                    response_bytes = _filecollection.GetFileContents(latest);
                }

                if (request.RawUrl.StartsWith(path + "index" + Constants.KOA))
                {
                    int index = 0;
                    string[] parts = request.RawUrl.Split("?index=");
                    Int32.TryParse(parts[1], out index);

                    Console.WriteLine($"Sending indexed file #[{index}] out of [{num_files}] total with filename [{_filecollection.GetFileNames()[index]}]");
                    response_bytes = _filecollection.GetFileContents(index);
                }

                if (response_bytes.Length > 0)
                {
                    response.OutputStream.Write(response_bytes);
                    response.OutputStream.Flush();
                }                
            }

            response.OutputStream.Close();
            Receive();
        }
    }
}