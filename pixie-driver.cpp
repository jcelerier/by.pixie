#include <ext.h>
#include <ext_obex.h>
#include <usb_driver.h>
#include <vector>
#undef error
#undef post
#define MAX_PIXIE_CHANNELS 510

struct by_pixie
{
    static t_class* theClass;

    t_object ob;

    int device_num;
    uint8_t output_config;
    std::unique_ptr<Pixie> pixie;

    //! Open the Pixie Driver
    void open_device()
    {
        post("Enttec PIXIE");

        FTDI_Reload();

        DWORD Num_Devices = 0;
        if (FT_CreateDeviceInfoList(&Num_Devices) == FT_OK)
        {
            post("Number of USB Devices found: %d", Num_Devices);
        }

        if (Num_Devices > 0)
        {
            for (int i = 0; i < Num_Devices; i++)
            {
                device_num = i;
                post("Testing Device: %d\n", device_num);

                DWORD Flags;
                DWORD ID;
                DWORD Type;
                DWORD LocId;
                char SerialNumber[16];
                char Description[64];
                FT_HANDLE DevHandler;

                auto status = FT_GetDeviceInfoDetail(0, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &DevHandler);
                if (status == FT_OK)
                {
                    post("SerialNumber: %s\n", SerialNumber);
                    post("Description: %s\n", Description);
                    FT_Close(DevHandler);
                }

                if ((pixie = createPixie(device_num)))
                    return;
            }
        }
    }

    //! Close the Pixie Driver
    void close_device()
    {
        if (pixie && pixie->handle)
            FTDI_ClosePort(pixie->handle);
    }

    //! Send a DMX message.
    static void dmx(by_pixie *x, t_symbol *s, long argc, t_atom *argv)
    {
        if (argc < 2)
            return;

        if (!x->pixie)
            return;

        t_atom *ap = argv;

        std::vector<uint8_t> dmx_data;
        dmx_data.reserve(MAX_PIXIE_CHANNELS + 1);

        // First get the universe
        if (atom_gettype(ap) != A_LONG)
            return;
        uint8_t universe = atom_getlong(ap);
        ap++;

        // Then the DMX values
        for (int i = 1; i < argc; i++, ap++)
        {
            switch (atom_gettype(ap))
            {
            case A_LONG:
            {
                auto val = atom_getlong(ap);
                dmx_data.push_back(atom_getlong(ap));
                break;
            }
            default:
                post("%ld: invalid atom type (%ld)", i + 1, atom_gettype(ap));
                break;
            }
        }

        x->write_dmx(universe, dmx_data);
    }

    static by_pixie* construct(t_symbol *s, long argc, t_atom *argv)
    {
        auto obj = (by_pixie*)object_alloc(by_pixie::theClass);
        if (obj)
        {
            obj->device_num = 0;
            obj->output_config = 1;
            new(&obj->pixie) std::unique_ptr<Pixie>{};
            obj->open_device();
        }
        return obj;
    }

    static void destruct(by_pixie* obj)
    {
        if (obj)
            obj->close_device();

        obj->pixie.~unique_ptr();
    }

private:
    void write_dmx(uint8_t universe, std::vector<uint8_t>& DMX)
    {
        if (DMX.size() < MAX_PIXIE_CHANNELS + 1)
            DMX.resize(MAX_PIXIE_CHANNELS + 1);

        DMX[MAX_PIXIE_CHANNELS] = 0 + (output_config << 2);

        if (universe > 0)
            DMX[MAX_PIXIE_CHANNELS] += (universe << 0);

        FTDI_SendData(pixie->handle, LED_UPDATE_LABEL, DMX.data(), MAX_PIXIE_CHANNELS + 1);
    }
};

t_class* by_pixie::theClass;


extern "C"
void ext_main(void *r)
{
    by_pixie::theClass = (t_class*)class_new(
        "by.pixie",
        (method)&by_pixie::construct,
        (method)&by_pixie::destruct,
        (long)sizeof(by_pixie), 0L, A_GIMME, 0);

    CLASS_ATTR_LONG(by_pixie::theClass, "config", 0, by_pixie, output_config);
    CLASS_ATTR_FILTER_CLIP(by_pixie::theClass, "config", 0, 2);
    CLASS_ATTR_LABEL(by_pixie::theClass, "config", 0, "LED strip output config");
    class_addmethod(by_pixie::theClass, (method)&by_pixie::dmx, "dmx", A_GIMME, 0);
    class_register(CLASS_BOX, by_pixie::theClass);

    post("by.pixie 1.0");
}
