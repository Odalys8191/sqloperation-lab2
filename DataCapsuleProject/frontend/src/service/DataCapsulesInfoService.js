import axios from 'axios';

export const DataCapsulesInfoService = {
    getData() {
        return axios
            .get('/api/cloud/getExistingDataCapsuleInformation', {
                params: { message: 'Request Data Capsules Info' }
            })
            .then((res) => res.data)
            .catch((err) => {
                console.error('Error fetching data:', err);
                return [];
            });
    },
    // // go gateway
    // getData() {
    //     return axios
    //         .get('/api/cloud/getExistingDataCapsuleInformation', {
    //             params: { message: 'Request Data Capsules Info' }
    //         })
    //         .then((res) => {
    //             const raw = res.data;

    //             try {
    //                 const capsules = JSON.parse(raw.dataCapsules);
    //                 return capsules;
    //             } catch (e) {
    //                 console.error('Error parsing dataCapsules:', e);
    //                 return [];
    //             }
    //         })
    //         .catch((err) => {
    //             console.error('Error fetching data:', err);
    //             return [];
    //         });
    // },
    getDataCapsulesSmall() {
        return this.getData().then((data) => data.slice(0, 10));
    },

    getDataCapsulesMedium() {
        return this.getData().then((data) => data.slice(0, 50));
    },

    getDataCapsulesLarge() {
        return this.getData().then((data) => data.slice(0, 200));
    },

    getDataCapsulesXLarge() {
        return this.getData();
    },

    // getDataCapsules(params) {
    //     const queryParams = params
    //         ? Object.keys(params)
    //             .map((k) => encodeURIComponent(k) + '=' + encodeURIComponent(params[k]))
    //             .join('&')
    //         : '';

    //     return axios
    //         .get('https://your-backend-api/datacapsules?' + queryParams)
    //         .then((res) => res.data)
    //         .catch((err) => {
    //             console.error('Error fetching datacapsules:', err);
    //             return [];
    //         });
    // }
};
