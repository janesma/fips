#include "publish.h"

#include <stddef.h>

#include "gfcpu_provider.h"
#include "gfpublisher_skel.h"
#include "gfpublisher.h"

using namespace Grafips;

CpuProvider *prov = NULL;

void create_publishers()
{
    prov = new CpuProvider;
    PublisherImpl *pub = new PublisherImpl;
    prov->setPublisher(pub);
    PublisherSkeleton *skel = new PublisherSkeleton(53136, pub);
    skel->Start();
}

void publish()
{
    if (prov)
        prov->Poll();
}
