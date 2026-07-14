import axios from 'axios';

export const DataCapsulesNumberService = {
    getNumber() {
        return axios
            .get('/api/cloud/getExistingDataCapsuleNumber', {
                params: { message: 'Request Data Capsules Number' }
            })
            .then((res) => res.data)
            .catch((err) => {
                console.error('Error fetching data:', err);
                return [];
            });
    },
};
