#include "publish.h"

#include <stddef.h>

#include "gfcpu_source.h"
#include "gfpublisher_skel.h"
#include "gfpublisher.h"

using namespace Grafips;

CpuSource *prov = NULL;

void create_publishers()
{
    prov = new CpuSource;
    PublisherImpl *pub = new PublisherImpl;
    prov->SetMetricSink(pub);
    PublisherSkeleton *skel = new PublisherSkeleton(53136, pub);
    skel->Start();
}

void publish()
{
    if (prov)
        prov->Poll();
}
