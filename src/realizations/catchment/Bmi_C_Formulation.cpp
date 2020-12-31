#include "Bmi_C_Formulation.hpp"

using namespace realization;
using namespace models::bmi;

Bmi_C_Formulation::Bmi_C_Formulation(std::string id, forcing_params forcing_config, utils::StreamHandler output_stream)
    : Bmi_Formulation<models::bmi::Bmi_C_Adapter>(id, forcing_config, output_stream) { }

std::string Bmi_C_Formulation::get_formulation_type() {
    return "bmi_c";
}

/**
 * Construct model and its shared pointer, potentially supplying input variable values from config.
 *
 * Construct a model (and a shared pointer to it), checking whether additional input variable values are present in the
 * configuration properties and need to be used during model construction.
 *
 * @param properties Configuration properties for the formulation, potentially containing values for input variables
 * @return A shared pointer to a newly constructed model adapter object
 */
std::shared_ptr<Bmi_C_Adapter> Bmi_C_Formulation::construct_model(const geojson::PropertyMap& properties) {
    // First examine properties to see if other input variable values are provided
    auto other_in_var_it = properties.find(BMI_REALIZATION_CFG_PARAM_OPT__OTHER_IN_VARS);
    if (other_in_var_it != properties.end()) {
        return std::make_shared<Bmi_C_Adapter>(
                Bmi_C_Adapter(
                        get_bmi_init_config(),
                        get_forcing_file_path(),
                        is_bmi_using_forcing_file(),
                        get_allow_model_exceed_end_time(),
                        other_in_var_it->second,
                        output));
    }
    else {
        return std::make_shared<Bmi_C_Adapter>(
                Bmi_C_Adapter(
                        get_bmi_init_config(),
                        get_forcing_file_path(),
                        is_bmi_using_forcing_file(),
                        get_allow_model_exceed_end_time(),
                        output));
    }
}

std::string Bmi_C_Formulation::get_output_header_line(std::string delimiter) {
    return boost::algorithm::join(get_output_header_fields(), delimiter);
}

std::string Bmi_C_Formulation::get_output_line_for_timestep(int timestep, std::string delimiter) {
    // Check if the timestep is in bounds for the fluxes vector, and handle case when it isn't
    if (timestep > get_bmi_model()->get_last_processed_time_step()) {
        return "";
    }
    std::string output_str;

    for (const std::string& name : get_output_variable_names()) {
        output_str += (output_str.empty() ? "" : ",") + std::to_string(get_var_value_as_double(timestep, name));
    }
    return output_str;
}

/**
 * Get the model response for this time step.
 *
 * Get the model response for this time step, execute the backing model formulation one or more times if the time step
 * of the given index has not already been processed.
 *
 * Function assumes the backing model has been fully initialized an that any additional input values have been applied.
 *
 * The function will return the value of the primary output variable (see `get_bmi_main_output_var()`) for the given
 * time step. The type returned will always be a `double`, with other numeric types being cast if necessary.
 *
 * Because of the nature of BMI, the `t_delta` parameter is ignored, as this cannot be passed meaningfully via
 * the `update()` BMI function.
 *
 * @param t_index The index of the time step for which to run model calculations.
 * @param d_delta_s The duration, in seconds, of the time step for which to run model calculations.
 * @return The total discharge of the model for the given time step.
 */
double Bmi_C_Formulation::get_response(time_step_t t_index, time_step_t t_delta) {
    if (get_bmi_model() == nullptr) {
        throw std::runtime_error("Trying to process response of improperly created BMI C formulation.");
    }
    if (t_index < 0) {
        throw std::invalid_argument("Getting response of negative time step in BMI C formulation is not allowed.");
    }

    int last_processed_time_step = get_bmi_model()->get_last_processed_time_step();
    while (last_processed_time_step++ < t_index) {
        get_bmi_model()->Update();
    }
    return get_var_value_as_double(t_index, get_bmi_main_output_var());
}

double Bmi_C_Formulation::get_var_value_as_double(time_step_t t_index, const std::string& var_name) {
    // TODO: change to using "at index" option
    std::string type = get_bmi_model()->GetVarType(var_name);
    if (type == "double") {
        std::vector<double> outputs = get_bmi_model()->GetValue<double>(var_name);
        return outputs[t_index];
    }
    else if (type == "float") {
        std::vector<float> outputs = get_bmi_model()->GetValue<float>(var_name);
        return (double)(outputs[t_index]);
    }
    else if (type == "int") {
        std::vector<int> outputs = get_bmi_model()->GetValue<int>(var_name);
        return (double)(outputs[t_index]);
    }
    else if (type == "long") {
        std::vector<long> outputs = get_bmi_model()->GetValue<long>(var_name);
        return (double)(outputs[t_index]);
    }
}

bool Bmi_C_Formulation::is_model_initialized() {
    return get_bmi_model()->is_model_initialized();
}
