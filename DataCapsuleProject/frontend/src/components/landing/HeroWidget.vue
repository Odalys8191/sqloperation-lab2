<template>
    <div id="hero" class="flex flex-col pt-6 px-6 lg:px-20 overflow-hidden"
        style="background: linear-gradient(0deg, rgba(255, 255, 255, 0.2), rgba(255, 255, 255, 0.2)), radial-gradient(77.36% 256.97% at 77.36% 57.52%, rgb(238, 239, 175) 0%, rgb(195, 227, 250) 100%); clip-path: ellipse(150% 87% at 93% 13%)">
        <div class="mx-6 md:mx-20 mt-0 md:mt-6">
            <h1 class="text-6xl font-bold text-gray-900 leading-tight"><span class="font-light block">数据胶囊</span>
                创新引领可信数据空间建设 </h1>
            <p class="font-normal text-2xl leading-normal md:mt-4 text-gray-700">
                本系统为可信数据空间提供有效解决方案和实践指导，构建“供得出、流得动、用得好、保安全”的数据生态，为数字经济发展提供源源不断的活力。</p>
            <Button label="开始使用" as="router-link" to="/" rounded class="!text-xl mt-8 !px-4"></Button>
        </div>
        <div class="flex justify-center md:justify-end">
            <img src="/demo/images/landing/screen-1.png" alt="Hero Image" class="w-9/12 md:w-auto" />
        </div>
    </div>
</template>
